// SPDX-License-Identifier: MIT
//
// Semantic file search with a vector store (the /vector_stores endpoints) — the
// retrieval half of a RAG setup, without running a model:
//   1. uploadFile()          — put a document into the Files API
//   2. createVectorStore()   — index it (ingestion runs server-side)
//   3. getVectorStore()      — poll until the file count says it is ready
//   4. searchVectorStore()   — ask a question and read the ranked chunks
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./vector_search [document] [query]

#include <QtOpenAi/Client/Client.h>

#include "ExampleSupport.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>

#include <functional>

using namespace QtOpenAi;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const std::optional<ExampleEnv> env = exampleEnv(out);
    if (!env)
        return 1;

    // Either index the document named on the command line or a built-in snippet.
    QByteArray document;
    QString fileName;
    if (argc > 1) {
        if (!readFileArg(QString::fromLocal8Bit(argv[1]), document, fileName, out))
            return 1;
    } else {
        document = QByteArray("QtOpenAi is a Qt 6 client library for OpenAI-compatible APIs.\n"
                              "To reset an API key, create a new one and revoke the old one.\n");
        fileName = QStringLiteral("qtopenai-notes.txt");
    }
    const QString query = argc > 2 ? QString::fromLocal8Bit(argv[2])
                                   : QStringLiteral("How do I reset an API key?");

    auto *client = new Client::Client(env->baseUrl, env->key, &app);

    auto reportError = failAndExit(out, app);

    // --- 4. search, once the store is ready ---------------------------------
    auto search = [&out, &app, client, query, reportError](const QString &storeId) {
        Core::VectorStoreSearchRequest request(query);
        request.setMaxNumResults(3);

        Client::VectorStoreSearchReply *reply = client->searchVectorStore(storeId, request);
        QObject::connect(reply, &Client::VectorStoreSearchReply::failed, reportError);
        QObject::connect(reply, &Client::VectorStoreSearchReply::finished,
                         [&out, &app](const Core::VectorStoreSearchPage &page) {
                             out << "\nResults (" << page.size() << "):\n";
                             for (const Core::VectorStoreSearchResult &hit : page.data) {
                                 out << "  [" << hit.score() << "] " << hit.filename() << "\n";
                                 out << "      " << hit.text() << "\n";
                             }
                             app.quit();
                         });
    };

    // --- 3. poll until ingestion finished -----------------------------------
    // Indexing is asynchronous, so the store reports in_progress until every
    // file has been chunked and embedded.
    auto pollStore = std::make_shared<std::function<void(const QString &)>>();
    *pollStore = [&out, client, reportError, search, pollStore](const QString &storeId) {
        Client::VectorStoreReply *reply = client->getVectorStore(storeId);
        QObject::connect(reply, &Client::VectorStoreReply::failed, reportError);
        QObject::connect(
                reply, &Client::VectorStoreReply::finished,
                [&out, storeId, search, pollStore](const Core::VectorStore &store) {
                    const Core::VectorStoreFileCounts counts = store.fileCounts();
                    if (counts.inProgress > 0) {
                        out << "  indexing: " << counts.completed << "/" << counts.total << "\n";
                        out.flush();
                        QTimer::singleShot(1000, [storeId, pollStore] { (*pollStore)(storeId); });
                        return;
                    }
                    out << "Store " << storeId << " ready (" << counts.completed
                        << " file(s) indexed, " << counts.failed << " failed).\n";
                    out.flush();
                    search(storeId);
                });
    };

    // --- 1./2. upload the document, then index it ---------------------------
    Core::FileUploadRequest upload(document, fileName, QStringLiteral("user_data"));
    Client::FileReply *uploaded = client->uploadFile(upload);
    QObject::connect(uploaded, &Client::FileReply::failed, reportError);

    QObject::connect(uploaded, &Client::FileReply::finished,
                     [&out, client, reportError, pollStore](const Core::FileObject &file) {
                         out << "Uploaded " << file.filename() << " as " << file.id() << "\n";
                         out.flush();

                         Core::CreateVectorStoreRequest request(QStringLiteral("qtopenai-example"),
                                                                {file.id()});
                         // Let the store clean itself up so repeated runs do not pile up.
                         request.setExpiresAfter(QStringLiteral("last_active_at"), 1);

                         Client::VectorStoreReply *created = client->createVectorStore(request);
                         QObject::connect(created, &Client::VectorStoreReply::failed, reportError);
                         QObject::connect(created, &Client::VectorStoreReply::finished,
                                          [pollStore](const Core::VectorStore &store) {
                                              (*pollStore)(store.id());
                                          });
                     });

    return app.exec();
}
