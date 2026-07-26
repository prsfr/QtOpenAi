// SPDX-License-Identifier: MIT
//
// Fine-tune a model (the /fine_tuning endpoints). Training is asynchronous and
// can run for a long time, so the flow is:
//   1. uploadFile()           — POST /files with purpose "fine-tune"
//   2. createFineTuningJob()  — POST /fine_tuning/jobs
//   3. pollFineTuningJob()    — GET /fine_tuning/jobs/{id} until terminal
//   4. listFineTuningEvents() — the progress log, once it is done
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./fine_tuning [path-to-training.jsonl]
//
// Without an argument a tiny built-in training set is used; note that real
// fine-tuning needs at least ten examples, so the generated one is only good
// for exercising the plumbing against a stub or a local server.

#include <QtOpenAi/Client/Client.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

// One line of a chat fine-tuning file: a complete conversation to imitate.
QByteArray trainingLine(const QString &prompt, const QString &answer)
{
    const QJsonObject line {
            {QStringLiteral("messages"), QJsonArray {Core::Message::user(prompt).toJson(),
                                                     Core::Message::assistant(answer).toJson()}},
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
    const QString model
            = env.value(QStringLiteral("OPENAI_MODEL"), QStringLiteral("gpt-4o-mini-2024-07-18"));
    if (apiKey.isEmpty()) {
        out << "Set OPENAI_API_KEY to run this example.\n";
        return 1;
    }

    QByteArray jsonl;
    if (argc > 1) {
        QFile file(QString::fromLocal8Bit(argv[1]));
        if (!file.open(QIODevice::ReadOnly)) {
            out << "Cannot read " << file.fileName() << "\n";
            return 1;
        }
        jsonl = file.readAll();
    } else {
        jsonl += trainingLine(QStringLiteral("What is Qt?"),
                              QStringLiteral("A cross-platform C++ application framework."));
        jsonl += trainingLine(QStringLiteral("What is QtOpenAi?"),
                              QStringLiteral("A Qt client library for the OpenAI API."));
    }

    auto *client = new Client::Client(QUrl(baseUrl), apiKey, &app);

    auto reportError = [&out, &app](const Client::ClientError &error) {
        out << "Error: " << error.message() << "\n";
        app.exit(1);
    };

    // 1. Training data lives in the Files API, with purpose "fine-tune".
    Core::FileUploadRequest upload(jsonl, QStringLiteral("training.jsonl"),
                                   QStringLiteral("fine-tune"));
    Client::FileReply *uploadReply = client->uploadFile(upload);
    QObject::connect(uploadReply, &Client::FileReply::failed, reportError);

    QObject::connect(
            uploadReply, &Client::FileReply::finished,
            [&out, &app, client, model, reportError](const Core::FileObject &trainingFile) {
                out << "Uploaded training data as " << trainingFile.id() << "\n";
                out.flush();

                // 2. Everything but the model and the file is optional; leaving
                // the hyperparameters unset lets the service choose them.
                Core::CreateFineTuningJobRequest request(model, trainingFile.id());
                request.setSuffix(QStringLiteral("qtopenai-example"));

                Client::FineTuningJobReply *created = client->createFineTuningJob(request);
                QObject::connect(created, &Client::FineTuningJobReply::failed, reportError);

                QObject::connect(
                        created, &Client::FineTuningJobReply::finished,
                        [&out, &app, client, reportError](const Core::FineTuningJob &job) {
                            out << "Created job " << job.id() << " ("
                                << Core::fineTuningJobStatusToString(job.status()) << ")\n";
                            out.flush();

                            // 3. Training takes minutes to hours; poll politely.
                            Client::FineTuningJobPoller *poller
                                    = client->pollFineTuningJob(job.id(), 30000);
                            QObject::connect(poller, &Client::FineTuningJobPoller::failed,
                                             reportError);

                            QObject::connect(poller, &Client::FineTuningJobPoller::progressed,
                                             [&out](const Core::FineTuningJob &state) {
                                                 out << "  "
                                                     << Core::fineTuningJobStatusToString(
                                                                state.status())
                                                     << "\n";
                                                 out.flush();
                                             });

                            QObject::connect(
                                    poller, &Client::FineTuningJobPoller::completed,
                                    [&out, &app, client,
                                     reportError](const Core::FineTuningJob &done) {
                                        if (done.status() != Core::FineTuningJobStatus::Succeeded) {
                                            out << "Job ended as "
                                                << Core::fineTuningJobStatusToString(done.status())
                                                << ": " << done.errorMessage() << "\n";
                                            app.quit();
                                            return;
                                        }
                                        out << "Fine-tuned model: " << done.fineTunedModel() << " ("
                                            << done.trainedTokens() << " trained tokens)\n";
                                        out.flush();

                                        // 4. The event log explains what happened.
                                        Client::ListParams params;
                                        params.limit = 10;
                                        Client::FineTuningEventListReply *events
                                                = client->listFineTuningEvents(done.id(), params);
                                        QObject::connect(events,
                                                         &Client::FineTuningEventListReply::failed,
                                                         reportError);
                                        QObject::connect(
                                                events, &Client::FineTuningEventListReply::finished,
                                                [&out,
                                                 &app](const Core::FineTuningJobEventList &list) {
                                                    out << "--- last events ---\n";
                                                    for (const Core::FineTuningJobEvent &event :
                                                         list.data)
                                                        out << "  [" << event.level() << "] "
                                                            << event.message() << "\n";
                                                    app.quit();
                                                });
                                    });

                            poller->start();
                        });
            });

    return app.exec();
}
