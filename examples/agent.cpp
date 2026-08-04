// SPDX-License-Identifier: MIT
//
// The tool loop, driven for you.
//
// examples/tool_loop.cpp writes the chat → tool_calls → tool results → chat
// loop out by hand: a recursive lambda over replies, with the history, the
// dispatch and the termination condition tangled together. Chat::Agent owns
// that loop, so this file only says what the tools are and what to ask:
//
//   Agent agent(&client, &registry);
//   agent.run("What is the weather in Berlin?");
//
// It also shows the three guards a loop that talks to a model needs: a cap on
// tool iterations, a wall-clock timeout, and an approval callback that can
// refuse a call before it happens -- with the refusal reported back to the
// model, so it says so instead of hanging.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./agent "What is the weather in Berlin and Hamburg?"
//   ./agent --ask "..."     # confirm each tool call on the terminal

#include <QtOpenAi/Chat/Agent.h>
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/ToolRegistry.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonObject>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

// The tool, as an ordinary invokable method -- its schema is derived from the
// signature (see examples/meta_tools.cpp).
class WeatherService : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    QTOPENAI_DOC_INVOKABLE(QJsonObject, forecast, "Get the current weather for a city.",
                           const QString &, location, "City name, e.g. Berlin")
    {
        return QJsonObject {
                {QStringLiteral("location"), location},
                {QStringLiteral("temp_c"), 21},
                {QStringLiteral("sky"), QStringLiteral("clear")},
        };
    }
};

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

    const QStringList arguments = app.arguments();
    const bool confirmEachCall = arguments.contains(QStringLiteral("--ask"));
    QString question = QStringLiteral("What is the weather in Berlin?");
    for (int i = 1; i < arguments.size(); ++i) {
        if (!arguments.at(i).startsWith(QStringLiteral("--")))
            question = arguments.at(i);
    }

    Client::Client client(QUrl(baseUrl), apiKey);

    WeatherService service;
    Client::ToolRegistry registry;
    registry.registerMethod(&service, QStringLiteral("forecast"));
    // A hallucinated argument comes back to the model as a message it can act
    // on, instead of reaching the method.
    registry.setValidateArguments(true);

    Chat::Agent agent(&client, &registry);
    agent.setModel(env.value(QStringLiteral("OPENAI_MODEL"), QStringLiteral("gpt-4o-mini")));
    agent.setSystemPrompt(QStringLiteral("You are a concise assistant."));
    agent.setStreaming(true);

    // The guards. Without them a model that keeps calling tools instead of
    // answering never stops, and a run that stalls never ends.
    agent.setMaxIterations(5);
    agent.setTimeoutMs(60000);

    if (confirmEachCall) {
        QTextStream in(stdin);
        agent.setApprovalCallback([&](const Core::ToolCall &call) {
            out << "\nRun " << call.function().name() << "(" << call.function().arguments()
                << ")? [y/N] ";
            out.flush();
            return in.readLine().trimmed().compare(QStringLiteral("y"), Qt::CaseInsensitive) == 0;
        });
    }

    QObject::connect(&agent, &Chat::Agent::contentDelta, [&out](const QString &text) {
        out << text;
        out.flush();
    });
    QObject::connect(&agent, &Chat::Agent::toolInvoked,
                     [&out](const QString &name, const QString &result) {
                         out << "\n  [tool] " << name << " -> " << result << "\n";
                         out.flush();
                     });
    QObject::connect(&agent, &Chat::Agent::toolRejected, [&out](const QString &name) {
        out << "\n  [tool] " << name << " declined\n";
        out.flush();
    });

    QObject::connect(&agent, &Chat::Agent::finished, [&](const Core::Message &) {
        out << "\n\n(" << agent.transcript().count() << " messages in the conversation)\n";
        app.quit();
    });
    QObject::connect(&agent, &Chat::Agent::failed, [&](const Client::ClientError &error) {
        out << "\nError: " << error.message() << "\n";
        app.exit(1);
    });

    agent.run(question);
    return app.exec();
}

#include "agent.moc"
