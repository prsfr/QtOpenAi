// SPDX-License-Identifier: MIT
//
// Send a multimodal message (text + image) to a vision-capable model. The user
// message is built from content parts (POST /chat/completions).
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./vision "https://example.com/photo.jpg" "What is in this image?"

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

    const QString imageUrl = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                      : QStringLiteral("https://upload.wikimedia.org/wikipedia/"
                                                       "commons/f/fd/Qt_logo_2016.svg");
    const QString prompt
            = argc > 2 ? QString::fromLocal8Bit(argv[2]) : QStringLiteral("What is in this image?");

    Client::Client client(env->baseUrl, env->key);

    // A user message with two content parts: the question and the image.
    const QList<Core::ContentPart> parts {
            Core::ContentPart::text(prompt),
            Core::ContentPart::imageUrl(imageUrl),
    };

    const QString model = env->model(QStringLiteral("gpt-4o-mini"));
    Core::ChatCompletionRequest request(model, {Core::Message::user(parts)});

    Client::ChatCompletionReply *reply = client.createChatCompletion(request);

    QObject::connect(reply, &Client::ChatCompletionReply::failed, failAndExit(out, app));

    QObject::connect(reply, &Client::ChatCompletionReply::finished,
                     [&out, &app](const Core::ChatCompletionResponse &response) {
                         out << response.firstMessage().content() << "\n";
                         app.quit();
                     });

    return app.exec();
}
