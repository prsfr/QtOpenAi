// SPDX-License-Identifier: MIT
//
// Smoke test for the installed CMake package: exercises both modules through
// the namespaced imported targets to prove headers, libraries and the config
// package all resolve.

#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/ToolRegistry.h>
#include <QtOpenAi/Core/ChatCompletionRequest.h>

#include <QtCore/QJsonObject>
#include <QtCore/QUrl>

using namespace QtOpenAi;

int main()
{
    Client::Client client(QUrl(QStringLiteral("http://localhost:1234/v1")),
                          QStringLiteral("test-key"));

    Core::ChatCompletionRequest request(QStringLiteral("gpt-4o"),
                                        {Core::Message::user(QStringLiteral("hi"))});

    Client::ToolRegistry registry;
    registry.registerFunction(QStringLiteral("noop"), QString(), QJsonObject {},
                              [](const QJsonObject &) { return QString(); });
    request.setTools(registry.tools());

    // Round-trip a request through JSON to touch the Core serialisation path.
    const bool ok
            = request.toJson().value(QStringLiteral("model")).toString() == QStringLiteral("gpt-4o")
              && registry.tools().size() == 1;
    return ok ? 0 : 1;
}
