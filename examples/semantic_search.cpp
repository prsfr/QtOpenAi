// SPDX-License-Identifier: MIT
//
// Retrieval-augmented generation with no vector database.
//
//   SemanticIndex index(&client);
//   index.add(paragraphs);                    // one request for the whole batch
//   auto *hits = index.query(question, 3);    // one request, then a local scan
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./semantic_search "what is the refund window?" notes.txt
//   ./semantic_search "what is the refund window?"      # a built-in corpus
//
// Indexes a corpus, retrieves the passages closest to the question, and asks
// the model to answer from those passages alone -- which is what "grounded in
// your own documents" means in practice.
//
// The index is saved to semantic_index.json and reloaded on the next run, so a
// second question does not pay to embed the same corpus again. That is the
// whole reason Core::VectorIndex is a serialisable value rather than internal
// state.

#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/SemanticIndex.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

constexpr QLatin1String kIndexFile("semantic_index.json");

QStringList corpus(const QString &path)
{
    if (path.isEmpty()) {
        return {QStringLiteral("Refunds are available within 30 days of purchase."),
                QStringLiteral("Support is reachable by email on weekdays, 9am to 5pm."),
                QStringLiteral("The device charges fully in about two hours."),
                QStringLiteral("Shipping is free on orders over fifty euro."),
                QStringLiteral("The warranty covers manufacturing defects for two years.")};
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    QStringList passages;
    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (!line.isEmpty())
            passages.append(line);
    }
    return passages;
}

Core::VectorIndex loadIndex()
{
    QFile file {QString(kIndexFile)};
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return Core::VectorIndex::fromJson(QJsonDocument::fromJson(file.readAll()).object());
}

void saveIndex(const Core::VectorIndex &index)
{
    QFile file {QString(kIndexFile)};
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(index.toJson()).toJson());
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

    const QString question = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                      : QStringLiteral("What is the refund window?");
    const QString model = env.value(QStringLiteral("OPENAI_MODEL"), QStringLiteral("gpt-4o-mini"));

    Client::Client client(QUrl(baseUrl), apiKey);
    Client::SemanticIndex index(&client);

    // Answer from the retrieved passages and nothing else: the point of
    // retrieval is that the answer can be traced to a source.
    const auto answer = [&](const QList<Core::VectorMatch> &hits) {
        if (hits.isEmpty()) {
            out << "Nothing in the corpus is close to that question.\n";
            app.quit();
            return;
        }

        QStringList context;
        out << "Retrieved:\n";
        for (const Core::VectorMatch &hit : hits) {
            out << "  [" << hit.score << "] " << hit.text << "\n";
            context.append(hit.text);
        }
        out << "\n";

        const QString prompt
                = QStringLiteral("Answer using only these passages. If they do not contain the "
                                 "answer, say so.\n\n%1\n\nQuestion: %2")
                          .arg(context.join(QStringLiteral("\n")), question);

        auto *reply = client.createChatCompletion(
                Core::ChatCompletionRequest(model, {Core::Message::user(prompt)}));
        QObject::connect(reply, &Client::ChatCompletionReply::finished,
                         [&out, &app](const Core::ChatCompletionResponse &response) {
                             out << response.choices().value(0).message().content() << "\n";
                             app.quit();
                         });
        QObject::connect(reply, &Client::ChatCompletionReply::failed,
                         [&out, &app](const Client::ClientError &error) {
                             out << "Error: " << error.message() << "\n";
                             app.quit();
                         });
    };

    const auto search = [&]() {
        auto *hits = index.query(question, 3);
        QObject::connect(hits, &Client::SemanticQueryReply::finished, answer);
        QObject::connect(hits, &Client::SemanticQueryReply::failed,
                         [&out, &app](const Client::ClientError &error) {
                             out << "Error: " << error.message() << "\n";
                             app.quit();
                         });
    };

    const Core::VectorIndex saved = loadIndex();
    if (!saved.isEmpty()) {
        out << "Reusing " << saved.size() << " indexed passages from " << kIndexFile << ".\n\n";
        index.setIndex(saved);
        search();
        return app.exec();
    }

    const QStringList passages = corpus(argc > 2 ? QString::fromLocal8Bit(argv[2]) : QString());
    if (passages.isEmpty()) {
        out << "Nothing to index.\n";
        return 1;
    }

    out << "Indexing " << passages.size() << " passages ...\n\n";
    auto *indexed = index.add(passages);
    QObject::connect(indexed, &Client::SemanticIndexReply::finished, [&](const QStringList &) {
        saveIndex(index.index());
        search();
    });
    QObject::connect(indexed, &Client::SemanticIndexReply::failed,
                     [&out, &app](const Client::ClientError &error) {
                         out << "Error: " << error.message() << "\n";
                         app.quit();
                     });

    return app.exec();
}
