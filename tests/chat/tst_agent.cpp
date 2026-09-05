// SPDX-License-Identifier: MIT
#include <QtOpenAi/Chat/Agent.h>
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/ToolRegistry.h>

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>
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

// An assistant turn asking for two tool calls, so a test can watch what
// happens to the second one after something has stopped the run during the
// first. Arguments are substituted for the same reason as above.
QByteArray twoToolCallsBody()
{
    const QString arguments = QStringLiteral("{\\\"location\\\":\\\"Berlin\\\"}");
    return QStringLiteral(R"({"id":"c","object":"chat.completion","created":1,
        "model":"gpt-4o-mini","choices":[{"index":0,"finish_reason":"tool_calls",
        "message":{"role":"assistant","content":null,"tool_calls":[
            {"id":"call_1","type":"function",
             "function":{"name":"get_weather","arguments":"%1"}},
            {"id":"call_2","type":"function",
             "function":{"name":"get_weather","arguments":"%1"}}]}}]})")
            .arg(arguments)
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
    void cancelAbortsTheTransferInFlight();
    void cancelStopsTheRequestAndFailsOnce();
    void cancelFromTheApprovalCallbackStopsTheToolLoop();
    void theDeadlineStopsARunBlockedInsideAToolCall();
    void aCancelledRunDoesNotDispatchIntoTheNextOne();

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

// The four below cover cancel() and the deadline. Both are documented in
// Agent.h as the reason the class exists, and both were untested until #161
// found that neither actually stopped anything.

void TestAgent::cancelAbortsTheTransferInFlight()
{
    // Stopping the run has to stop the transfer, not just stop listening to
    // it: a provider that is still generating is still charging. StubServer
    // answers at once, so this needs a server that accepts and then says
    // nothing, leaving the request genuinely in flight to be aborted.
    QTcpServer silent;
    QVERIFY(silent.listen(QHostAddress::LocalHost, 0));

    bool accepted = false;
    bool disconnected = false;
    connect(&silent, &QTcpServer::newConnection, this, [&] {
        QTcpSocket *socket = silent.nextPendingConnection();
        accepted = true;
        connect(socket, &QTcpSocket::disconnected, this, [&] { disconnected = true; });
    });

    Client client(QUrl(QStringLiteral("http://127.0.0.1:%1/v1").arg(silent.serverPort())),
                  QStringLiteral("k"));
    client.setRetryPolicy(RetryPolicy::none());

    Agent agent(&client, nullptr);
    agent.setModel(QStringLiteral("gpt-4o-mini"));

    QVERIFY(agent.run(QStringLiteral("hi")));
    QTRY_VERIFY_WITH_TIMEOUT(accepted, 5000);
    QVERIFY(!disconnected);

    agent.cancel();

    // Without a reachable abort the connection just stays open.
    QTRY_VERIFY_WITH_TIMEOUT(disconnected, 5000);
}

void TestAgent::cancelStopsTheRequestAndFailsOnce()
{
    StubServer server(answerBody());
    Client client(server.baseUrl(), QStringLiteral("k"));
    // An aborted transfer must not be retried into a second request.
    client.setRetryPolicy(RetryPolicy::none());

    Agent agent(&client, nullptr);
    agent.setModel(QStringLiteral("gpt-4o-mini"));

    QSignalSpy failedSpy(&agent, &Agent::failed);
    QSignalSpy finishedSpy(&agent, &Agent::finished);

    QVERIFY(agent.run(QStringLiteral("hi")));
    agent.cancel();

    // Exactly one failure: aborting makes the reply fail too, and that failure
    // is not a second thing to report.
    QCOMPARE(failedSpy.count(), 1);
    QVERIFY(!agent.isRunning());

    // Give the abandoned reply every chance to come back with an answer.
    QTest::qWait(300);
    QCOMPARE(finishedSpy.count(), 0);
    QCOMPARE(failedSpy.count(), 1);
}

void TestAgent::cancelFromTheApprovalCallbackStopsTheToolLoop()
{
    // The approval callback is application code called from inside the tool
    // loop, and cancelling from it is exactly what a Stop button does.
    StubServer server(QList<StubServer::Response> {{twoToolCallsBody()}, {answerBody()}});
    Client client(server.baseUrl(), QStringLiteral("k"));

    int toolCalls = 0;
    ToolRegistry *registry = weatherRegistry(this, &toolCalls);

    Agent agent(&client, registry);
    agent.setModel(QStringLiteral("gpt-4o-mini"));

    int approvals = 0;
    agent.setApprovalCallback([&agent, &approvals](const ToolCall &) {
        if (++approvals == 1)
            agent.cancel();
        return true;
    });

    QSignalSpy failedSpy(&agent, &Agent::failed);
    QSignalSpy invokedSpy(&agent, &Agent::toolInvoked);

    QVERIFY(agent.run(QStringLiteral("go")));
    QVERIFY(failedSpy.wait(5000));
    QTest::qWait(300);

    // The run stopped where it was told to: the approved call did not run, the
    // second call was never even offered for approval, and no further turn was
    // dispatched.
    QCOMPARE(approvals, 1);
    QCOMPARE(toolCalls, 0);
    QCOMPARE(invokedSpy.count(), 0);
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(server.requestCount(), 1);
    QVERIFY(!agent.isRunning());
}

void TestAgent::theDeadlineStopsARunBlockedInsideAToolCall()
{
    // A tool that blocks on a nested event loop delivers the agent's own
    // deadline while it waits -- HttpTools' http_get documents itself as
    // working exactly this way, so this needs no application code to reach.
    StubServer server(QList<StubServer::Response> {{twoToolCallsBody()}, {answerBody()}});
    Client client(server.baseUrl(), QStringLiteral("k"));
    client.setRetryPolicy(RetryPolicy::none());

    int toolCalls = 0;
    auto *registry = new ToolRegistry(this);
    registry->registerFunction(QStringLiteral("get_weather"), QStringLiteral("Weather"),
                               QJsonObject {}, [&toolCalls](const QJsonObject &) {
                                   ++toolCalls;
                                   QEventLoop inner;
                                   QTimer::singleShot(300, &inner, &QEventLoop::quit);
                                   inner.exec();
                                   return QStringLiteral("sunny");
                               });

    Agent agent(&client, registry);
    agent.setModel(QStringLiteral("gpt-4o-mini"));
    agent.setTimeoutMs(150);

    QSignalSpy failedSpy(&agent, &Agent::failed);
    QVERIFY(agent.run(QStringLiteral("go")));
    QVERIFY(failedSpy.wait(5000));
    QTest::qWait(400);

    // The first tool was already running when the deadline fired, so it
    // finishes; the second must not start, and the run must not go on to ask
    // the model again -- that request would be unguarded, since the deadline
    // has been stopped.
    QCOMPARE(toolCalls, 1);
    QCOMPARE(server.requestCount(), 1);
    QCOMPARE(failedSpy.count(), 1);
    QVERIFY(failedSpy.first().first().value<ClientError>().message().contains(
            QStringLiteral("exceeded")));
    QVERIFY(!agent.isRunning());
}

void TestAgent::aCancelledRunDoesNotDispatchIntoTheNextOne()
{
    // The composed failure: a cancelled run that still dispatches leaves a
    // reply nobody owns, and the next run answers the previous question.
    // Only two responses are queued, and the queue repeats its last entry. A
    // run that dispatched after being cancelled would burn the second one on
    // the orphan request, so the count below is what catches it.
    StubServer server(QList<StubServer::Response> {{twoToolCallsBody()},
                                                   {answerBody(QStringLiteral("second answer"))}});
    Client client(server.baseUrl(), QStringLiteral("k"));

    int toolCalls = 0;
    ToolRegistry *registry = weatherRegistry(this, &toolCalls);

    Agent agent(&client, registry);
    agent.setModel(QStringLiteral("gpt-4o-mini"));

    bool cancelled = false;
    agent.setApprovalCallback([&agent, &cancelled](const ToolCall &) {
        if (!cancelled) {
            cancelled = true;
            agent.cancel();
        }
        return true;
    });

    QSignalSpy failedSpy(&agent, &Agent::failed);
    QVERIFY(agent.run(QStringLiteral("first")));
    QVERIFY(failedSpy.wait(5000));
    QTest::qWait(200);

    // cancel() invites a fresh run: isRunning() is false, so run() is accepted.
    agent.setApprovalCallback(nullptr);
    QSignalSpy finishedSpy(&agent, &Agent::finished);
    QVERIFY(agent.run(QStringLiteral("second")));
    QVERIFY(finishedSpy.wait(5000));

    // Two requests in total, not three: the cancelled run dispatched nothing,
    // so the answer the application is shown is the answer to its own prompt.
    QCOMPARE(server.requestCount(), 2);
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.first().first().value<Message>().content(),
             QStringLiteral("second answer"));
}

QTEST_MAIN(TestAgent)
#include "tst_agent.moc"
