// SPDX-License-Identifier: MIT
//
// Structured Outputs bound to a C++ type.
//
// examples/structured_output.cpp spells the JSON-Schema out and prints the raw
// reply. Here one Q_GADGET does both ends: its Q_PROPERTYs become the schema
// the model is constrained to, and the reply is read straight back into an
// instance of it. Rename a property and both sides follow.
//
//   1. ResponseFormat::forType<Person>() -- schema from the gadget
//   2. MetaJson::parse<Person>(content)  -- reply back into the gadget
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./typed_output "John Doe is 30 and lives in Berlin."
//
// Pass --schema to print the generated response_format and exit, which needs no
// API key.

#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Core/MetaJson.h>
#include <QtOpenAi/Core/MetaSchema.h>

#include "ExampleSupport.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

// The shape of the answer, as a type. The QTOPENAI_DOC macros carry the
// descriptions the meta-object system has no room for; they expand to the
// Q_CLASSINFO keys MetaSchema reads, without spelling the paths out here.
class Person
{
    Q_GADGET
    QTOPENAI_DOC("A person mentioned in the text")
    QTOPENAI_DOC_PROPERTY(age, "Age in whole years")
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(int age MEMBER age)
    Q_PROPERTY(QString city MEMBER city)
public:
    QString name;
    int age = 0;
    QString city;
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    // Strict mode wants every property required and the object closed, which is
    // what MetaSchema emits -- so nothing has to be said here.
    const Core::ResponseFormat format = Core::ResponseFormat::forType<Person>();

    if (app.arguments().contains(QStringLiteral("--schema"))) {
        out << QJsonDocument(format.toJson()).toJson(QJsonDocument::Indented);
        return 0;
    }

    const std::optional<ExampleEnv> env
            = exampleEnv(out, KeyKind::Api, QStringLiteral("pass --schema"));
    if (!env)
        return 1;

    const QString input = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                   : QStringLiteral("John Doe is 30 and lives in Berlin.");

    Client::Client client(env->baseUrl, env->key);

    const QString model = env->model(QStringLiteral("gpt-4o-mini"));
    Core::ChatCompletionRequest request(
            model, {Core::Message::user(QStringLiteral("Extract the person from: ") + input)});
    request.setResponseFormat(format);

    Client::ChatCompletionReply *reply = client.createChatCompletion(request);

    QObject::connect(reply, &Client::ChatCompletionReply::failed, failAndExit(out, app));

    QObject::connect(reply, &Client::ChatCompletionReply::finished,
                     [&out, &app](const Core::ChatCompletionResponse &response) {
                         // Typed from here on: no JSON left to unpack by hand.
                         const Person person
                                 = Core::MetaJson::parse<Person>(response.firstMessage().content());
                         out << person.name << ", " << person.age << ", " << person.city << "\n";
                         app.quit();
                     });

    return app.exec();
}

#include "typed_output.moc"
