// SPDX-License-Identifier: MIT
//
// Create a response with the modern Responses API (POST /responses) and print
// its aggregated output text.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./responses "Give me one fun fact about the Qt framework."

#include <QtOpenAi/Client/Client.h>

#include "ExampleSupport.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const std::optional<ExampleEnv> env = exampleEnv(out);
    if (!env)
        return 1;

    const QString input = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                   : QStringLiteral("Give me one fun fact about the Qt framework.");

    Client::Client client(env->baseUrl, env->key);

    const QString model = env->model(QStringLiteral("gpt-4o-mini"));
    Core::ResponseRequest request(model, input);

    Client::ResponseReply *reply = client.createResponse(request);

    QObject::connect(reply, &Client::ResponseReply::failed, failAndExit(out, app));

    QObject::connect(reply, &Client::ResponseReply::finished,
                     [&out, &app](const Core::Response &response) {
                         out << response.outputText() << "\n";
                         app.quit();
                     });

    return app.exec();
}
