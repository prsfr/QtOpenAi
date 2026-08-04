// SPDX-License-Identifier: MIT
//
// Conversations, the response cache and the metrics, kept across sessions.
//
// Run it twice. The first run has nothing to restore and asks the model; the
// second finds the conversation from the first, replays the same question out
// of the cache without a round trip, and reports totals that include both runs.
// That is the whole feature: what a desktop application would otherwise lose
// every time it is closed.
//
//   Storage::JsonFileStore store(directory);   // or Sql::SqliteStore(file)
//   store.open();
//
// One store holds all three. Everything else is the ordinary API -- Transcript,
// CachingInterceptor, MetricsCollector -- with a store behind it:
//
//   * conversations   store.saveConversation(id, transcript) / loadConversation
//   * cache           PersistentResponseCache, handed to CachingInterceptor
//   * metrics         store.saveMetrics(id, snapshot) / MetricsCollector::restore
//
// Autosave writes on a timer instead of on every change: a save per streamed
// fragment is a file write per fragment, and a save on exit is the one that is
// missing after a crash.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./persistence "Why is the sky blue?"
//
// The store lives in a "qtopenai-persistence-example" directory under the
// user's application-data location; QTOPENAI_STORE overrides it. Pass --sqlite
// to use the SQLite backend when it was built.

#include <QtOpenAi/Chat/Transcript.h>
#include <QtOpenAi/Client/CachingInterceptor.h>
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/MetricsCollector.h>
#include <QtOpenAi/Storage/Autosave.h>
#include <QtOpenAi/Storage/JsonFileStore.h>
#include <QtOpenAi/Storage/PersistentResponseCache.h>

#ifdef QTOPENAI_EXAMPLES_HAVE_SQL
#include <QtOpenAi/Sql/SqliteStore.h>
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>

#include <memory>

using namespace QtOpenAi;

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

    const QStringList arguments = app.arguments();
    const bool useSqlite = arguments.contains(QStringLiteral("--sqlite"));
    QString question = QStringLiteral("Why is the sky blue?");
    for (int i = 1; i < arguments.size(); ++i) {
        if (!arguments.at(i).startsWith(QStringLiteral("--")))
            question = arguments.at(i);
    }
    const QString model = env.value(QStringLiteral("OPENAI_MODEL"), QStringLiteral("gpt-4o-mini"));

    const QString root = env.value(QStringLiteral("QTOPENAI_STORE"),
                                   QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                           + QStringLiteral("/qtopenai-persistence-example"));

    // The one line an application changes to swap backends; everything below
    // this point is written against the Store interface.
    std::unique_ptr<Storage::Store> store;
    if (useSqlite) {
#ifdef QTOPENAI_EXAMPLES_HAVE_SQL
        store = std::make_unique<Sql::SqliteStore>(root + QStringLiteral("/history.db"));
#else
        out << "This build has no QtOpenAi::Sql module; using the JSON-files store.\n";
        store = std::make_unique<Storage::JsonFileStore>(root);
#endif
    } else {
        store = std::make_unique<Storage::JsonFileStore>(root);
    }

    if (!store->open()) {
        out << "Cannot open the store: " << store->lastError() << "\n";
        return 1;
    }
    out << "Store: " << root << " (schema version " << store->schemaVersion() << ")\n";

    const QString conversationId = QStringLiteral("example");

    // --- Restore what the last run left behind -----------------------------
    Chat::Transcript transcript;
    if (const std::optional<Chat::Transcript> saved = store->loadConversation(conversationId)) {
        transcript = *saved;
        out << "Restored " << transcript.count() << " messages from the last run.\n";
    } else {
        transcript.setSystemPrompt(QStringLiteral("You are terse."));
        out << "No conversation stored yet; starting a new one.\n";
    }

    Client::MetricsCollector metrics;
    if (const std::optional<Client::MetricsSnapshot> saved
        = store->loadMetrics(QStringLiteral("all-time"))) {
        // Restored rather than added to: the snapshot already contains those
        // requests, and counting them twice is how a total stops being one.
        metrics.restore(*saved);
        out << "Restored metrics: " << saved->requests << " requests so far.\n";
    }

    // --- Wire the client ---------------------------------------------------
    Client::Client client(QUrl(baseUrl), apiKey);
    metrics.attach(&client);

    Storage::PersistentResponseCache cache(store.get());
    Client::CachingInterceptor caching;
    caching.setCache(&cache);
    client.addInterceptor(&caching);
    QObject::connect(&caching, &Client::CachingInterceptor::hit, [&out](const QUrl &url) {
        out << "Cache hit for " << url.path() << " -- no request was made.\n";
    });

    // --- Autosave ----------------------------------------------------------
    Storage::Autosave autosave(store.get());
    autosave.setIntervalMs(2000);
    autosave.setConversation(conversationId, [&transcript] { return transcript; });
    autosave.setMetrics(QStringLiteral("all-time"), &metrics);
    QObject::connect(&autosave, &Storage::Autosave::failed, [&out](const QString &error) {
        // A silent autosave failure is data loss nobody hears about.
        out << "Autosave failed: " << error << "\n";
    });

    transcript.addUserMessage(question);
    autosave.touch();

    out << "\n> " << question << "\n";
    Client::ChatCompletionReply *reply
            = client.createChatCompletion(transcript.buildRequest(model));
    metrics.observe(reply);

    QObject::connect(reply, &Client::ChatCompletionReply::failed,
                     [&](const Client::ClientError &error) {
                         out << "Error: " << error.message() << "\n";
                         app.quit();
                     });

    QObject::connect(reply, &Client::ChatCompletionReply::finished,
                     [&](const Core::ChatCompletionResponse &response) {
                         transcript.addMessage(response.firstMessage());
                         out << response.firstMessage().content() << "\n";
                         autosave.touch();

                         // Flushed here rather than in Autosave's destructor:
                         // the conversation is fetched through a callback into
                         // this program, and calling it while the program is
                         // being torn down reads objects that may be gone.
                         if (!autosave.flush())
                             out << "The final save did not go through.\n";

                         const Client::MetricsSnapshot snapshot = metrics.snapshot();
                         out << "\n--- all time ---\n";
                         out << "Requests: " << snapshot.requests
                             << ", cached bodies held: " << cache.count() << "\n";
                         out << "Tokens:   " << snapshot.totals().totalTokens << "\n";
                         out << "Cost:     $" << snapshot.cost() << "\n";
                         out << "Run it again: the same question is answered from the store.\n";
                         app.quit();
                     });

    return app.exec();
}
