// SPDX-License-Identifier: MIT
//
// Demonstrates a full tool-calling round trip:
//   1. Register a local "get_weather" tool with the ToolRegistry.
//   2. Send a user question plus the advertised tools.
//   3. If the model requests tool calls, dispatch them through the registry
//      (via Qt's meta-object / signal-slot machinery) and send the results
//      back for a final answer.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./chat_tool_loop "What's the weather in Berlin?"
//
// Point it at any OpenAI-compatible endpoint via OPENAI_BASE_URL, e.g. a local
// Ollama instance: export OPENAI_BASE_URL=http://localhost:11434/v1

#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/ToolRegistry.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
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
                                      : QStringLiteral("What is the weather in Berlin?");

    Client::Client client(QUrl(baseUrl), apiKey);

    // --- Register a local tool -------------------------------------------
    Client::ToolRegistry registry;
    const QJsonObject weatherSchema {
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("properties"),
             QJsonObject {
                     {QStringLiteral("location"),
                      QJsonObject {{QStringLiteral("type"), QStringLiteral("string")},
                                   {QStringLiteral("description"), QStringLiteral("City name")}}}}},
            {QStringLiteral("required"), QJsonArray {QStringLiteral("location")}},
    };
    registry.registerFunction(
            QStringLiteral("get_weather"), QStringLiteral("Get the current weather for a city."),
            weatherSchema, [](const QJsonObject &args) {
                const QString location = args.value(QStringLiteral("location")).toString();
                return QStringLiteral("{\"location\":\"%1\",\"temp_c\":21,\"sky\":\"clear\"}")
                        .arg(location);
            });

    QObject::connect(&registry, &Client::ToolRegistry::toolInvoked,
                     [&out](const QString &, const QString &name, const QString &result) {
                         out << "  [tool] " << name << " -> " << result << "\n";
                         out.flush();
                     });

    // --- Build the initial request ---------------------------------------
    Core::ChatCompletionRequest request(QStringLiteral("gpt-4o-mini"),
                                        {Core::Message::user(question)});
    request.setTools(registry.tools());

    // Drive the conversation with a small recursive lambda over replies.
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
                                 // Feed the assistant turn + tool results back.
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
