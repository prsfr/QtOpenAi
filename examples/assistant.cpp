// SPDX-License-Identifier: MIT
//
// The Assistants beta end to end (/assistants + /threads): a server-side
// assistant, a thread that keeps the transcript for you, and a run that calls a
// local tool along the way.
//   1. createAssistant()    — POST /assistants, advertising the ToolRegistry's tools
//   2. createThread()       — POST /threads, seeded with the user's question
//   3. createRun()          — POST /threads/{id}/runs
//   4. pollRun()            — until the run finishes, or asks for tool outputs
//   5. submitToolOutputs()  — dispatch the calls through the ToolRegistry and
//                             hand the results back; the run continues
//   6. listThreadMessages() — the answer, plus the transcript it was built from
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./assistant "What is the weather in Oslo?"
//
// Unlike the chat tool loop, the conversation itself lives on the server: the
// thread id is the only state this program keeps between turns. Because a run
// can park on `requires_action` more than once, the steps are members of a small
// state holder rather than nested lambdas — the flow is a cycle, not a ladder.
// The assistant and the thread are deleted again at the end, so re-running
// leaves nothing behind.

#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/ToolRegistry.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

class AssistantDemo
{
public:
    AssistantDemo(Client::Client *client, QString model, QString question)
        : m_client(client)
        , m_model(std::move(model))
        , m_question(std::move(question))
    {
        registerTools();
    }

    // 1. The assistant is a stored configuration: model, instructions, tools.
    void start()
    {
        Core::CreateAssistantRequest request(m_model);
        request.setName(QStringLiteral("Weather assistant"));
        request.setInstructions(QStringLiteral(
                "You answer weather questions. Use the get_weather tool and reply in one "
                "sentence."));
        for (const Core::Tool &tool : m_registry.tools())
            request.addTool(tool);

        Client::AssistantReply *reply = m_client->createAssistant(request);
        watch(reply);
        QObject::connect(reply, &Client::AssistantReply::finished,
                         [this](const Core::Assistant &assistant) {
                             m_assistantId = assistant.id();
                             print(QStringLiteral("Created assistant %1 (%2)")
                                           .arg(assistant.id(), assistant.name()));
                             openThread();
                         });
    }

private:
    void registerTools()
    {
        // The same registry the chat tool loop uses: a run hands back ordinary
        // ToolCalls, so nothing about local dispatch changes here.
        const QJsonObject schema {
                {QStringLiteral("type"), QStringLiteral("object")},
                {QStringLiteral("properties"),
                 QJsonObject {{QStringLiteral("location"),
                               QJsonObject {{QStringLiteral("type"), QStringLiteral("string")},
                                            {QStringLiteral("description"),
                                             QStringLiteral("City name")}}}}},
                {QStringLiteral("required"), QJsonArray {QStringLiteral("location")}},
        };
        m_registry.registerFunction(
                QStringLiteral("get_weather"),
                QStringLiteral("Get the current weather for a city."), schema,
                [](const QJsonObject &arguments) {
                    const QString location = arguments.value(QStringLiteral("location")).toString();
                    return QStringLiteral("{\"location\":\"%1\",\"temp_c\":8,\"sky\":\"rain\"}")
                            .arg(location);
                });
    }

    // 2. A thread holds the conversation server-side; seed it with the question
    // so the run has something to answer.
    void openThread()
    {
        Core::CreateThreadRequest request;
        request.addUserMessage(m_question);

        Client::ThreadReply *reply = m_client->createThread(request);
        watch(reply);
        QObject::connect(reply, &Client::ThreadReply::finished, [this](const Core::Thread &thread) {
            m_threadId = thread.id();
            print(QStringLiteral("Created thread %1").arg(thread.id()));
            startRun();
        });
    }

    // 3. Run the assistant against the thread.
    void startRun()
    {
        const Core::CreateRunRequest request(m_assistantId);
        Client::RunReply *reply = m_client->createRun(m_threadId, request);
        watch(reply);
        QObject::connect(reply, &Client::RunReply::finished, [this](const Core::Run &run) {
            print(QStringLiteral("Started run %1").arg(run.id()));
            followRun(run.id());
        });
    }

    // 4. Wait for the run to settle. It stops for two reasons, and the poller
    // reports them separately.
    void followRun(const QString &runId)
    {
        Client::RunPoller *poller = m_client->pollRun(m_threadId, runId, 1000);
        QObject::connect(poller, &Client::RunPoller::failed,
                         [this](const Client::ClientError &e) { fail(e); });
        QObject::connect(poller, &Client::RunPoller::progressed, [this](const Core::Run &state) {
            print(QStringLiteral("  %1").arg(Core::runStatusToString(state.status())));
        });
        QObject::connect(poller, &Client::RunPoller::requiresAction,
                         [this](const Core::Run &run) { answerToolCalls(run); });
        QObject::connect(poller, &Client::RunPoller::completed,
                         [this](const Core::Run &run) { readTranscript(run); });
        poller->start();
    }

    // 5. The run parked on a tool call: dispatch it locally, hand the result
    // back, and follow the run it continues into.
    void answerToolCalls(const Core::Run &run)
    {
        QList<Core::ToolOutput> outputs;
        for (const Core::ToolCall &call : run.requiredToolCalls()) {
            const Core::Message result = m_registry.invoke(call);
            print(QStringLiteral("  [tool] %1 -> %2")
                          .arg(call.function().name(), result.content()));
            outputs.append({call.id(), result.content()});
        }

        Client::RunReply *reply = m_client->submitToolOutputs(m_threadId, run.id(), outputs);
        watch(reply);
        QObject::connect(reply, &Client::RunReply::finished,
                         [this](const Core::Run &continued) { followRun(continued.id()); });
    }

    // 6. Read the finished conversation back, then clean up.
    void readTranscript(const Core::Run &run)
    {
        print(QStringLiteral("Run %1 %2 (%3 tokens)")
                      .arg(run.id(), Core::runStatusToString(run.status()))
                      .arg(run.usage().totalTokens()));

        Client::ThreadMessageListReply *reply = m_client->listThreadMessages(m_threadId);
        watch(reply);
        QObject::connect(
                reply, &Client::ThreadMessageListReply::finished,
                [this](const Core::ThreadMessageList &list) {
                    print(QStringLiteral("--- transcript ---"));
                    // The list is most-recent-first, so walk it
                    // backwards to read in conversation order.
                    for (int i = list.data.size() - 1; i >= 0; --i) {
                        const Core::ThreadMessage &message = list.data.at(i);
                        print(QStringLiteral("  %1: %2")
                                      .arg(Core::roleToString(message.role()), message.text()));
                    }
                    cleanUp();
                });
    }

    // Both resources outlive the program otherwise. They are deleted one after
    // the other rather than at once: quitting on whichever finished first would
    // abort the other request mid-flight and leave it behind.
    void cleanUp()
    {
        Client::ThreadReply *thread = m_client->deleteThread(m_threadId);
        QObject::connect(thread, &Client::ThreadReply::done, [this] {
            Client::AssistantReply *assistant = m_client->deleteAssistant(m_assistantId);
            QObject::connect(assistant, &Client::AssistantReply::done, qApp,
                             &QCoreApplication::quit);
        });
    }

    // Every request in the chain reports a failure the same way.
    template <typename Reply>
    void watch(Reply *reply)
    {
        QObject::connect(reply, &Reply::failed,
                         [this](const Client::ClientError &error) { fail(error); });
    }

    void fail(const Client::ClientError &error)
    {
        print(QStringLiteral("Error: %1").arg(error.message()));
        qApp->exit(1);
    }

    void print(const QString &line)
    {
        QTextStream out(stdout);
        out << line << "\n";
    }

    Client::Client *m_client;
    Client::ToolRegistry m_registry;
    QString m_model;
    QString m_question;
    QString m_assistantId;
    QString m_threadId;
};

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString apiKey = env.value(QStringLiteral("OPENAI_API_KEY"));
    const QString baseUrl = env.value(QStringLiteral("OPENAI_BASE_URL"),
                                      QStringLiteral("https://api.openai.com/v1"));
    const QString model = env.value(QStringLiteral("OPENAI_MODEL"), QStringLiteral("gpt-4o-mini"));
    if (apiKey.isEmpty()) {
        out << "Set OPENAI_API_KEY to run this example.\n";
        return 1;
    }

    const QString question = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                      : QStringLiteral("What is the weather in Oslo?");

    Client::Client client(QUrl(baseUrl), apiKey, &app);
    AssistantDemo demo(&client, model, question);
    demo.start();

    return app.exec();
}
