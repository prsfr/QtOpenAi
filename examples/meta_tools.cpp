// SPDX-License-Identifier: MIT
//
// Tool calling without a hand-written schema.
//
// examples/tool_loop.cpp registers a tool by spelling out its JSON-Schema and
// unpacking the arguments object by hand. Here the same tool is an ordinary
// Q_INVOKABLE method: the schema is derived from its signature, the arguments
// are checked against that schema before dispatch, and the parameters arrive
// as C++ types.
//
//   1. Q_INVOKABLE QString forecast(const QString &location, int days)
//   2. registerMethod(&service, "forecast") -- name, parameters and description
//      all come from the meta-object, so nothing can drift apart.
//   3. setValidateArguments(true) -- a hallucinated argument comes back to the
//      model as a message naming the problem, instead of reaching the method.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./meta_tools "What is the weather in Berlin over the next 3 days?"
//
// Pass --schema to print the generated tool definition and exit, which needs no
// API key.

#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/ToolRegistry.h>
#include <QtOpenAi/Core/MetaSchema.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

// The tool, as plain C++. The Q_CLASSINFO annotations are the one thing the
// meta-object system does not already know: what the method and its arguments
// mean. Each is addressed by its path -- "doc:<method>", then
// "doc:<method>:<argument>".
class WeatherService : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("doc:forecast", "Get the weather forecast for a city.")
    Q_CLASSINFO("doc:forecast:location", "City name, e.g. Berlin")
    Q_CLASSINFO("doc:forecast:days", "How many days ahead to forecast, 1 to 7")
public:
    using QObject::QObject;

    // Returning a QJsonObject is enough; the registry serialises it into the
    // tool result.
    Q_INVOKABLE QJsonObject forecast(const QString &location, int days)
    {
        return QJsonObject {
                {QStringLiteral("location"), location},
                {QStringLiteral("days"), days},
                {QStringLiteral("temp_c"), 21},
                {QStringLiteral("sky"), QStringLiteral("clear")},
        };
    }
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    WeatherService service;
    Client::ToolRegistry registry;
    // Everything the model is told about the tool comes from the method above.
    registry.registerMethod(&service, QStringLiteral("forecast"));
    // Arguments are checked against that same schema before the method runs.
    registry.setValidateArguments(true);

    const QStringList arguments = app.arguments();
    if (arguments.contains(QStringLiteral("--schema"))) {
        const Core::Tool tool = registry.tools().first();
        QJsonObject definition {
                {QStringLiteral("name"), tool.function().name()},
                {QStringLiteral("description"), tool.function().description()},
                {QStringLiteral("parameters"), tool.function().parameters()},
        };
        out << QJsonDocument(definition).toJson(QJsonDocument::Indented);
        return 0;
    }

    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString apiKey = env.value(QStringLiteral("OPENAI_API_KEY"));
    const QString baseUrl = env.value(QStringLiteral("OPENAI_BASE_URL"),
                                      QStringLiteral("https://api.openai.com/v1"));
    if (apiKey.isEmpty()) {
        out << "Set OPENAI_API_KEY to run this example, or pass --schema.\n";
        return 1;
    }

    const QString question
            = argc > 1 ? QString::fromLocal8Bit(argv[1])
                       : QStringLiteral("What is the weather in Berlin over the next 3 days?");

    Client::Client client(QUrl(baseUrl), apiKey);

    QObject::connect(&registry, &Client::ToolRegistry::toolInvoked,
                     [&out](const QString &, const QString &name, const QString &result) {
                         out << "  [tool] " << name << " -> " << result << "\n";
                         out.flush();
                     });
    // A rejected call is not a failure of the program: the message goes back to
    // the model, which gets to try again with what it now knows.
    QObject::connect(&registry, &Client::ToolRegistry::argumentsRejected,
                     [&out](const QString &, const QString &name, const QStringList &errors) {
                         out << "  [tool] " << name
                             << " rejected: " << errors.join(QStringLiteral("; ")) << "\n";
                         out.flush();
                     });

    const QString model = env.value(QStringLiteral("OPENAI_MODEL"), QStringLiteral("gpt-4o-mini"));
    Core::ChatCompletionRequest request(model, {Core::Message::user(question)});
    request.setTools(registry.tools());

    std::function<void(Core::ChatCompletionRequest)> send;
    send = [&](Core::ChatCompletionRequest req) {
        Client::ChatCompletionReply *reply = client.createChatCompletion(req);

        QObject::connect(reply, &Client::ChatCompletionReply::failed,
                         [&out, &app](const Client::ClientError &error) {
                             out << "Error: " << error.message() << "\n";
                             app.exit(1);
                         });

        QObject::connect(reply, &Client::ChatCompletionReply::finished,
                         [&, req](const Core::ChatCompletionResponse &response) mutable {
                             const Core::Message message = response.firstMessage();
                             if (!message.toolCalls().isEmpty()) {
                                 req.addMessage(message);
                                 for (const Core::Message &result :
                                      registry.invokeAll(message.toolCalls())) {
                                     req.addMessage(result);
                                 }
                                 send(req);
                                 return;
                             }
                             out << "\nAssistant: " << message.content() << "\n";
                             app.quit();
                         });
    };

    send(request);
    return app.exec();
}

#include "meta_tools.moc"
