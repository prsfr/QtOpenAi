// SPDX-License-Identifier: MIT
//
// Smoke test for the installed CMake package: exercises every module through
// the namespaced imported targets to prove headers, libraries and the config
// package all resolve.

#include <QtOpenAi/Chat/Transcript.h>
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/ToolRegistry.h>
#include <QtOpenAi/Core/ChatCompletionRequest.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonObject>
#include <QtCore/QUrl>

#ifdef QTOPENAI_CONSUMER_HAS_REALTIME
#include <QtOpenAi/Realtime/RealtimeConnection.h>
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

    // Round-trip a request through JSON to touch the Core serialisation path.
    const bool ok
            = request.toJson().value(QStringLiteral("model")).toString() == QStringLiteral("gpt-4o")
              && registry.tools().size() == 1
              && transcript.buildRequest(QStringLiteral("gpt-4o")).messages().size() == 2;
    return ok ? 0 : 1;
}
