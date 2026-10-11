// SPDX-License-Identifier: MIT
//
// Constrain the model to a JSON schema (Structured Outputs) so the reply is
// guaranteed to match the requested shape (POST /chat/completions with
// response_format = json_schema).
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./structured_output "John Doe is 30 and lives in Berlin."

#include <QtOpenAi/Client/Client.h>

#include "ExampleSupport.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
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
                                   : QStringLiteral("John Doe is 30 and lives in Berlin.");

    Client::Client client(env->baseUrl, env->key);

    // A schema for {name, age, city}. Strict mode requires all properties listed
    // in `required` and additionalProperties disabled.
    const QJsonObject schema {
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("properties"),
             QJsonObject {
                     {QStringLiteral("name"),
                      QJsonObject {{QStringLiteral("type"), QStringLiteral("string")}}},
                     {QStringLiteral("age"),
                      QJsonObject {{QStringLiteral("type"), QStringLiteral("integer")}}},
                     {QStringLiteral("city"),
                      QJsonObject {{QStringLiteral("type"), QStringLiteral("string")}}},
             }},
            {QStringLiteral("required"),
             QJsonArray {QStringLiteral("name"), QStringLiteral("age"), QStringLiteral("city")}},
            {QStringLiteral("additionalProperties"), false},
    };

    const QString model = env->model(QStringLiteral("gpt-4o-mini"));
    Core::ChatCompletionRequest request(
            model, {Core::Message::user(QStringLiteral("Extract the person from: ") + input)});
    request.setResponseFormat(Core::ResponseFormat::jsonSchema(QStringLiteral("person"), schema));

    Client::ChatCompletionReply *reply = client.createChatCompletion(request);

    QObject::connect(reply, &Client::ChatCompletionReply::failed, failAndExit(out, app));

    QObject::connect(reply, &Client::ChatCompletionReply::finished,
                     [&out, &app](const Core::ChatCompletionResponse &response) {
                         // The content is JSON guaranteed to match the schema.
                         out << response.firstMessage().content() << "\n";
                         app.quit();
                     });

    return app.exec();
}
