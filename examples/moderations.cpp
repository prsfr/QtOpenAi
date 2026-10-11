// SPDX-License-Identifier: MIT
//
// Classify text against the moderation policy (POST /moderations) and report
// whether it was flagged.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./moderations "some text to check"

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

    const QString text
            = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("I want to hug a puppy.");

    Client::Client client(env->baseUrl, env->key);

    Core::ModerationRequest request(text);
    // Optional: override the moderation model (e.g. for a local endpoint).
    const QString model = env->model({});
    if (!model.isEmpty())
        request.setModel(model);

    Client::ModerationReply *reply = client.createModeration(request);

    QObject::connect(reply, &Client::ModerationReply::failed, failAndExit(out, app));

    QObject::connect(reply, &Client::ModerationReply::finished,
                     [&out, &app](const Core::ModerationResponse &response) {
                         const Core::ModerationResult result = response.firstResult();
                         out << "flagged: " << (result.flagged() ? "yes" : "no") << "\n";
                         app.quit();
                     });

    return app.exec();
}
