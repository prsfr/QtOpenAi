// SPDX-License-Identifier: MIT
#include <QtOpenAi/Chat/Agent.h>
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/ToolRegistry.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;
using namespace QtOpenAi::Chat;

namespace {

// An assistant turn that asks for one tool call. The arguments field is JSON
// inside a JSON string, and it is substituted rather than written inline:
// moc's lexer does not read escaped quotes inside a raw string.
QByteArray toolCallBody(const QString &id = QStringLiteral("call_1"))
{
    const QString arguments = QStringLiteral("{\\\"location\\\":\\\"Berlin\\\"}");
    return QStringLiteral(R"({"id":"c","object":"chat.completion","created":1,
        "model":"gpt-4o-mini","choices":[{"index":0,"finish_reason":"tool_calls",
        "message":{"role":"assistant","content":null,"tool_calls":[
            {"id":"%1","type":"function",
             "function":{"name":"get_weather","arguments":"%2"}}]}}]})")
            .arg(id, arguments)
            .toUtf8();
}

// A plain answer, which is what ends a run.
QByteArray answerBody(const QString &text = QStringLiteral("It is sunny."))
{
    return QStringLiteral(R"({"id":"c","object":"chat.completion","created":1,
        "model":"gpt-4o-mini","choices":[{"index":0,"finish_reason":"stop",
        "message":{"role":"assistant","content":"%1"}}]})")
            .arg(text)
            .toUtf8();
}

} // namespace

// Coverage for the agent loop (#38). The loop itself is simple; the guards are
// the reason it belongs in a library rather than in every application, so most
// of this is about the ways a run has to be able to stop.
class TestAgent : public QObject
{
    Q_OBJECT
private slots:
    void runsToolCallsAndThenAnswers();
    void answersWithoutToolsInOneTurn();
    void stopsAtTheIterationLimit();
    void refusedToolsAreReportedBackToTheModel();
    void reportsATransportFailure();
    void refusesASecondConcurrentRun();
    void accumulatesTheConversationAcrossRuns();

private:
    static ToolRegistry *weatherRegistry(QObject *parent, int *calls);
};

ToolRegistry *TestAgent::weatherRegistry(QObject *parent, int *calls)
{
    auto *registry = new ToolRegistry(parent);
    registry->registerFunction(QStringLiteral("get_weather"), QStringLiteral("Weather"),
                               QJsonObject {}, [calls](const QJsonObject &args) {
                                   ++(*calls);
                                   return QStringLiteral("sunny in %1")
                                           .arg(args.value(QStringLiteral("location")).toString());
                               });
    return registry;
}

void TestAgent::runsToolCallsAndThenAnswers()
{
    // The whole point: one call drives request → tool → request → answer.
    StubServer server(QList<StubServer::Response> {{toolCallBody()}, {answerBody()}});
    Client client(server.baseUrl(), QStringLiteral("k"));

    int toolCalls = 0;
    ToolRegistry *registry = weatherRegistry(this, &toolCalls);

    Agent agent(&client, registry);
    agent.setModel(QStringLiteral("gpt-4o-mini"));
    agent.setSystemPrompt(QStringLiteral("be terse"));

    QSignalSpy assistantSpy(&agent, &Agent::assistantMessage);
    QSignalSpy toolSpy(&agent, &Agent::toolInvoked);
    QSignalSpy finishedSpy(&agent, &Agent::finished);

    QVERIFY(agent.run(QStringLiteral("weather in Berlin?")));
    QVERIFY(agent.isRunning());
    QVERIFY(finishedSpy.wait(5000));

    QCOMPARE(toolCalls, 1);
    QCOMPARE(toolSpy.count(), 1);
    QCOMPARE(toolSpy.first().at(0).toString(), QStringLiteral("get_weather"));
    QCOMPARE(toolSpy.first().at(1).toString(), QStringLiteral("sunny in Berlin"));
    // Both assistant turns are announced -- the one that asked for tools too.
    QCOMPARE(assistantSpy.count(), 2);
    QCOMPARE(finishedSpy.first().first().value<Message>().content(),
             QStringLiteral("It is sunny."));
    QVERIFY(!agent.isRunning());

    // The transcript holds the whole exchange: prompt, request, result, answer.
    QCOMPARE(agent.transcript().count(), 4);
    // ... and the tools were advertised, or the model could not have asked.
    QVERIFY(server.requestBodies().value(0).contains("\"tools\""));
}

void TestAgent::answersWithoutToolsInOneTurn()
{
    StubServer server(answerBody(QStringLiteral("42")));
    Client client(server.baseUrl(), QStringLiteral("k"));

    // A null registry is a plain chat loop, not an error.
    Agent agent(&client, nullptr);
    agent.setModel(QStringLiteral("gpt-4o-mini"));

    QSignalSpy finishedSpy(&agent, &Agent::finished);
    QVERIFY(agent.run(QStringLiteral("what is 6 times 7?")));
    QVERIFY(finishedSpy.wait(5000));

    QCOMPARE(finishedSpy.first().first().value<Message>().content(), QStringLiteral("42"));
    QCOMPARE(agent.transcript().count(), 2);
}

void TestAgent::stopsAtTheIterationLimit()
{
    // A model that keeps asking for tools instead of answering must not loop
    // for ever; the run ends as a failure, which is the honest outcome.
    StubServer server(toolCallBody());
    Client client(server.baseUrl(), QStringLiteral("k"));

    int toolCalls = 0;
    ToolRegistry *registry = weatherRegistry(this, &toolCalls);

    Agent agent(&client, registry);
    agent.setModel(QStringLiteral("gpt-4o-mini"));
    agent.setMaxIterations(2);

    QSignalSpy failedSpy(&agent, &Agent::failed);
    QSignalSpy finishedSpy(&agent, &Agent::finished);

    QVERIFY(agent.run(QStringLiteral("loop please")));
    QVERIFY(failedSpy.wait(5000));

    QCOMPARE(finishedSpy.count(), 0);
    QCOMPARE(toolCalls, 2);
    QVERIFY(failedSpy.first().first().value<ClientError>().message().contains(
            QStringLiteral("2 tool iterations")));
    QVERIFY(!agent.isRunning());
}

void TestAgent::refusedToolsAreReportedBackToTheModel()
{
    StubServer server(QList<StubServer::Response> {
            {toolCallBody()}, {answerBody(QStringLiteral("I could not check."))}});
    Client client(server.baseUrl(), QStringLiteral("k"));

    int toolCalls = 0;
    ToolRegistry *registry = weatherRegistry(this, &toolCalls);

    Agent agent(&client, registry);
    agent.setModel(QStringLiteral("gpt-4o-mini"));
    agent.setApprovalCallback([](const ToolCall &) { return false; });

    QSignalSpy rejectedSpy(&agent, &Agent::toolRejected);
    QSignalSpy invokedSpy(&agent, &Agent::toolInvoked);
    QSignalSpy finishedSpy(&agent, &Agent::finished);

    QVERIFY(agent.run(QStringLiteral("weather in Berlin?")));
    QVERIFY(finishedSpy.wait(5000));

    // The handler never ran ...
    QCOMPARE(toolCalls, 0);
    QCOMPARE(invokedSpy.count(), 0);
    QCOMPARE(rejectedSpy.count(), 1);
    // ... but the model was told, so the run reached an answer rather than
    // hanging on a tool call nobody answered.
    QCOMPARE(finishedSpy.count(), 1);
    const QByteArray secondRequest = server.requestBodies().value(1);
    QVERIFY(secondRequest.contains("not permitted to run"));
}

void TestAgent::reportsATransportFailure()
{
    StubServer server(500, R"({"error":{"message":"boom","type":"server_error"}})");
    Client client(server.baseUrl(), QStringLiteral("k"));
    client.setRetryPolicy(RetryPolicy::none());

    Agent agent(&client, nullptr);
    agent.setModel(QStringLiteral("gpt-4o-mini"));

    QSignalSpy failedSpy(&agent, &Agent::failed);
    QVERIFY(agent.run(QStringLiteral("hi")));
    QVERIFY(failedSpy.wait(5000));

    QCOMPARE(failedSpy.first().first().value<ClientError>().httpStatus(), 500);
    QVERIFY(!agent.isRunning());
}

void TestAgent::refusesASecondConcurrentRun()
{
    // One agent, one conversation: a second run would interleave two exchanges
    // in the same transcript.
    StubServer server(answerBody());
    Client client(server.baseUrl(), QStringLiteral("k"));

    Agent agent(&client, nullptr);
    agent.setModel(QStringLiteral("gpt-4o-mini"));

    QVERIFY(agent.run(QStringLiteral("first")));
    QVERIFY(!agent.run(QStringLiteral("second")));
    QVERIFY(!agent.resume());

    QSignalSpy finishedSpy(&agent, &Agent::finished);
    QVERIFY(finishedSpy.wait(5000));
    // The refused prompt was not appended either.
    QCOMPARE(agent.transcript().count(), 2);
}

void TestAgent::accumulatesTheConversationAcrossRuns()
{
    StubServer server(QList<StubServer::Response> {{answerBody(QStringLiteral("one"))},
                                                   {answerBody(QStringLiteral("two"))}});
    Client client(server.baseUrl(), QStringLiteral("k"));

    Agent agent(&client, nullptr);
    agent.setModel(QStringLiteral("gpt-4o-mini"));

    QSignalSpy finishedSpy(&agent, &Agent::finished);
    QVERIFY(agent.run(QStringLiteral("first")));
    QVERIFY(finishedSpy.wait(5000));
    QVERIFY(agent.run(QStringLiteral("second")));
    QVERIFY(finishedSpy.wait(5000));

    // The second request carried the first exchange, which is what makes it a
    // conversation rather than two calls.
    QCOMPARE(agent.transcript().count(), 4);
    QVERIFY(server.requestBodies().value(1).contains("\"one\""));
    QCOMPARE(finishedSpy.at(1).first().value<Message>().content(), QStringLiteral("two"));
}

QTEST_MAIN(TestAgent)
#include "tst_agent.moc"
