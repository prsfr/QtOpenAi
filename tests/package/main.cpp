// SPDX-License-Identifier: MIT
//
// Smoke test for the installed CMake package: exercises every module through
// the namespaced imported targets to prove headers, libraries and the config
// package all resolve.

#include <QtOpenAi/Admin/Organization.h>
#include <QtOpenAi/Chat/Transcript.h>
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/ToolRegistry.h>
#include <QtOpenAi/Core/ChatCompletionRequest.h>
#include <QtOpenAi/Core/VectorIndex.h>
#include <QtOpenAi/Storage/JsonFileStore.h>
#include <QtOpenAi/Tools/DefaultTools.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonObject>
#include <QtCore/QTemporaryDir>
#include <QtCore/QUrl>

#ifdef QTOPENAI_CONSUMER_HAS_REALTIME
#include <QtOpenAi/Realtime/RealtimeConnection.h>
#endif

#ifdef QTOPENAI_CONSUMER_HAS_SQL
#include <QtOpenAi/Sql/SqliteStore.h>
#endif

using namespace QtOpenAi;

int main(int argc, char **argv)
{
    // The library's QObject types expect an application object, the same as any
    // Qt program using them would have.
    QCoreApplication app(argc, argv);

#ifdef QTOPENAI_CONSUMER_HAS_REALTIME
    // The optional third module, when the package was built with it.
    Realtime::RealtimeConnection connection;
    connection.setModel(QStringLiteral("gpt-realtime"));
    if (connection.isOpen())
        return 1;
#endif

    Client::Client client(QUrl(QStringLiteral("http://localhost:1234/v1")),
                          QStringLiteral("test-key"));

    Core::ChatCompletionRequest request(QStringLiteral("gpt-4o"),
                                        {Core::Message::user(QStringLiteral("hi"))});

    Client::ToolRegistry registry;
    registry.registerFunction(QStringLiteral("noop"), QString(), QJsonObject {},
                              [](const QJsonObject &) { return QString(); });
    request.setTools(registry.tools());

    // The Chat module, which builds a request without any networking.
    Chat::Transcript transcript;
    transcript.setSystemPrompt(QStringLiteral("be terse"));
    transcript.addUserMessage(QStringLiteral("hi"));

    // The Tools module. An empty policy installs nothing, which is both the
    // documented default and the cheapest thing to assert here.
    Client::ToolRegistry sandboxed;
    Tools::DefaultTools tools;
    const int granted = tools.install(&sandboxed, Tools::ToolPolicy {}).size();

    // A Core type with no networking behind it at all.
    Core::VectorIndex index;
    index.add(QStringLiteral("a"), {1.0, 0.0});

    // The Admin module. Configured but not called: the administration surface
    // needs an admin key and a server, and this smoke test has neither.
    Admin::Organization organization(QUrl(QStringLiteral("http://localhost:1234/v1")),
                                     QStringLiteral("sk-admin-test"));
    Admin::UsageQuery usageQuery;
    usageQuery.startTime = 1730419200;
    Core::CreateInviteRequest invitation(QStringLiteral("a@example.com"), QStringLiteral("reader"));
    const bool adminReady = organization.adminKey() == QStringLiteral("sk-admin-test")
                            && usageQuery.toQuery().hasQueryItem(QStringLiteral("start_time"))
                            && invitation.toJson().contains(QStringLiteral("role"));

    // The Storage module, against a directory that goes away with this process.
    QTemporaryDir storeRoot;
    Storage::JsonFileStore store(storeRoot.path());
    const bool stored = store.open() && store.saveConversation(QStringLiteral("c"), transcript)
                        && store.loadConversation(QStringLiteral("c")).has_value();

#ifdef QTOPENAI_CONSUMER_HAS_SQL
    // The optional SQLite backend, when the package was built with it. In
    // memory, so the smoke test leaves nothing behind.
    Sql::SqliteStore database(QStringLiteral(":memory:"));
    if (!database.open() || !database.saveConversation(QStringLiteral("c"), transcript))
        return 1;
#endif

    // Round-trip a request through JSON to touch the Core serialisation path.
    const bool ok
            = request.toJson().value(QStringLiteral("model")).toString() == QStringLiteral("gpt-4o")
              && registry.tools().size() == 1 && granted == 0 && index.size() == 1
              && index.search({1.0, 0.0}, 1).size() == 1
              && transcript.buildRequest(QStringLiteral("gpt-4o")).messages().size() == 2 && stored
              && adminReady;
    return ok ? 0 : 1;
}
