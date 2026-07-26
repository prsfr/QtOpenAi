// SPDX-License-Identifier: MIT
//
// Run a batch job (the /batches endpoints). Batches trade latency for cost: a
// whole JSONL file of requests is processed asynchronously within a completion
// window, at a discount. The full round trip is:
//   1. uploadFile()          — POST /files with purpose "batch"
//   2. createBatch()         — POST /batches
//   3. pollBatch()           — GET /batches/{id} until terminal
//   4. downloadFileContent() — GET /files/{output_file_id}/content
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./batch [prompt ...]           # defaults to two small demo prompts
//
// Note that a real batch can take up to its completion window (24h) to finish;
// this example simply keeps polling until it does.

#include <QtOpenAi/Client/Client.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

// One line of the batch input file: a custom id the results are correlated by,
// the HTTP method and route, plus the request body that route expects.
QByteArray requestLine(const QString &customId, const QString &model, const QString &prompt)
{
    Core::Message message = Core::Message::user(prompt);
    QJsonObject body {
            {QStringLiteral("model"), model},
            {QStringLiteral("messages"), QJsonArray {message.toJson()}},
            {QStringLiteral("max_tokens"), 64},
    };
    const QJsonObject line {
            {QStringLiteral("custom_id"), customId},
            {QStringLiteral("method"), QStringLiteral("POST")},
            {QStringLiteral("url"), QStringLiteral("/v1/chat/completions")},
            {QStringLiteral("body"), body},
    };
    return QJsonDocument(line).toJson(QJsonDocument::Compact) + "\n";
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString apiKey = env.value(QStringLiteral("OPENAI_API_KEY"));
    const QString baseUrl = env.value(QStringLiteral("OPENAI_BASE_URL"),
                                      QStringLiteral("https://api.openai.com/v1"));
    if (apiKey.isEmpty()) {
        out << "Set OPENAI_API_KEY to run this example.\n";
        return 1;
    }

    const QString model = QStringLiteral("gpt-4o-mini");

    // Either batch the prompts given on the command line or two demo ones.
    QStringList prompts;
    for (int i = 1; i < argc; ++i)
        prompts << QString::fromLocal8Bit(argv[i]);
    if (prompts.isEmpty())
        prompts << QStringLiteral("Name one fact about Qt.")
                << QStringLiteral("Name one fact about C++.");

    QByteArray jsonl;
    for (int i = 0; i < prompts.size(); ++i)
        jsonl += requestLine(QStringLiteral("request-%1").arg(i + 1), model, prompts.at(i));

    auto *client = new Client::Client(QUrl(baseUrl), apiKey, &app);

    auto reportError = [&out, &app](const Client::ClientError &error) {
        out << "Error: " << error.message() << "\n";
        app.exit(1);
    };

    // 1. The input file must live in the Files API, with purpose "batch".
    Core::FileUploadRequest upload(jsonl, QStringLiteral("batch-input.jsonl"),
                                   QStringLiteral("batch"));
    Client::FileReply *uploadReply = client->uploadFile(upload);
    QObject::connect(uploadReply, &Client::FileReply::failed, reportError);

    QObject::connect(
            uploadReply, &Client::FileReply::finished,
            [&out, &app, client, reportError](const Core::FileObject &inputFile) {
                out << "Uploaded input as " << inputFile.id() << " (" << inputFile.bytes()
                    << " bytes)\n";
                out.flush();

                // 2. Queue the batch against the endpoint every line targets.
                Core::CreateBatchRequest request(inputFile.id(),
                                                 QStringLiteral("/v1/chat/completions"));
                Client::BatchReply *created = client->createBatch(request);
                QObject::connect(created, &Client::BatchReply::failed, reportError);

                QObject::connect(
                        created, &Client::BatchReply::finished,
                        [&out, &app, client, reportError](const Core::Batch &batch) {
                            out << "Created batch " << batch.id() << " ("
                                << Core::batchStatusToString(batch.status()) << ")\n";
                            out.flush();

                            // 3. Poll until the batch stops changing. Ten seconds
                            // is polite for a job measured in minutes or hours.
                            Client::BatchPoller *poller = client->pollBatch(batch.id(), 10000);
                            QObject::connect(poller, &Client::BatchPoller::failed, reportError);

                            QObject::connect(poller, &Client::BatchPoller::progressed,
                                             [&out](const Core::Batch &state) {
                                                 const Core::BatchRequestCounts counts
                                                         = state.requestCounts();
                                                 out << "  "
                                                     << Core::batchStatusToString(state.status())
                                                     << " — " << counts.completed << "/"
                                                     << counts.total << " done, " << counts.failed
                                                     << " failed\n";
                                                 out.flush();
                                             });

                            QObject::connect(
                                    poller, &Client::BatchPoller::completed,
                                    [&out, &app, client, reportError](const Core::Batch &done) {
                                        if (done.status() != Core::BatchStatus::Completed) {
                                            out << "Batch ended as "
                                                << Core::batchStatusToString(done.status()) << "\n";
                                            for (const Core::BatchError &error : done.errors())
                                                out << "  " << error.code << ": " << error.message
                                                    << "\n";
                                            app.quit();
                                            return;
                                        }

                                        // 4. The results are a JSONL file again,
                                        // one response line per input line.
                                        Client::BinaryReply *output
                                                = client->downloadFileContent(done.outputFileId());
                                        QObject::connect(output, &Client::BinaryReply::failed,
                                                         reportError);
                                        QObject::connect(output, &Client::BinaryReply::finished,
                                                         [&out, &app](const QByteArray &bytes) {
                                                             out << "--- results ---\n"
                                                                 << bytes << "\n";
                                                             app.quit();
                                                         });
                                    });

                            poller->start();
                        });
            });

    return app.exec();
}
