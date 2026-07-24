// SPDX-License-Identifier: MIT
//
// Stream a chat completion token by token (Server-Sent Events). The reply emits
// contentDelta() for each text chunk and finished() with the assembled response.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./streaming "Write a haiku about Qt."

#include <QtOpenAi/Client/Client.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

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

    const QString question = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                      : QStringLiteral("Write a haiku about Qt.");

    Client::Client client(QUrl(baseUrl), apiKey);

    const QString model = env.value(QStringLiteral("OPENAI_MODEL"), QStringLiteral("gpt-4o-mini"));
    Core::ChatCompletionRequest request(model, {Core::Message::user(question)});

    Client::ChatCompletionStreamReply *stream = client.createChatCompletionStream(request);

    QObject::connect(stream, &Client::ChatCompletionStreamReply::contentDelta,
                     [&out](const QString &text) {
                         out << text;
                         out.flush();
                     });

    QObject::connect(stream, &Client::ChatCompletionStreamReply::failed,
                     [&out, &app](const Client::ClientError &error) {
                         out << "\nError: " << error.message() << "\n";
                         app.exit(1);
                     });

    QObject::connect(stream, &Client::ChatCompletionStreamReply::finished,
                     [&out, &app](const Core::ChatCompletionResponse &) {
                         out << "\n";
                         app.quit();
                     });

    return app.exec();
}
