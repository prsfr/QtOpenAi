// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

// A throwaway HTTP/1.1 server that streams a canned Server-Sent-Events body and
// closes, so the run-stream reply can be exercised offline.
class RunSseStubServer : public QObject
{
    Q_OBJECT
public:
    explicit RunSseStubServer(QByteArray body, QObject *parent = nullptr)
        : QObject(parent)
        , m_body(std::move(body))
    {
        m_server.listen(QHostAddress::LocalHost, 0);
        connect(&m_server, &QTcpServer::newConnection, this, &RunSseStubServer::onConnection);
    }

    QUrl baseUrl() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1/v1").arg(m_server.serverPort()));
    }

    QByteArray requestHead() const { return m_request.left(m_request.indexOf("\r\n\r\n")); }
    QByteArray requestBody() const { return m_request.mid(m_request.indexOf("\r\n\r\n") + 4); }

private slots:
    void onConnection()
    {
        QTcpSocket *socket = m_server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            m_request += socket->readAll();
            if (!m_request.contains("\r\n\r\n"))
                return;
            const QByteArray response = "HTTP/1.1 200 OK\r\n"
                                        "Content-Type: text/event-stream\r\n"
                                        "Content-Length: "
                                        + QByteArray::number(m_body.size())
                                        + "\r\n"
                                          "Connection: close\r\n\r\n"
                                        + m_body;
            socket->write(response);
            socket->flush();
            socket->disconnectFromHost();
        });
    }

private:
    QTcpServer m_server;
    QByteArray m_body;
    QByteArray m_request;
};

// Offline stub-server coverage for the thread half of the Assistants beta (#24):
// threads, messages, the run lifecycle including the submit-tool-outputs turn,
// run steps, the poller and the streamed run.
class TestThreadsClient : public QObject
{
    Q_OBJECT
private slots:
    void createsThreadWithSeedMessages();
    void getUpdateDeleteThread();
    void createsMessage();
    void listsMessagesFilteredByRun();
    void getUpdateDeleteMessage();
    void createsRunBelowThread();
    void createsThreadAndRunInOneCall();
    void listsRuns();
    void cancelPostsToCancelSubPath();
    void toolLoopRunsToCompletion();
    void listsAndGetsRunSteps();
    void pollerStopsOnRequiredAction();
    void pollerFollowsRunToCompletion();
    void streamsMessageDeltasAndRunStates();
    void streamReportsOnlySettledMessages();
    void streamSurfacesErrorEvents();
    void streamedToolOutputsResumeTheRun();
    void updateRunPostsMetadata();
};

void TestThreadsClient::createsThreadWithSeedMessages()
{
    StubServer server(QByteArray(R"({"id":"thread_1","object":"thread","created_at":1716028800})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateThreadRequest request;
    request.addUserMessage(QStringLiteral("What is the weather in Oslo?"));

    const auto reply = awaited(client.createThread(request));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/threads "));
    QVERIFY(server.requestHeaders().toLower().contains("openai-beta: assistants=v2"));
    QVERIFY(server.requestBody().contains("\"content\":\"What is the weather in Oslo?\""));
    QCOMPARE(reply->thread().id(), QStringLiteral("thread_1"));

    // The default argument creates an empty thread with an empty body.
    StubServer bare(QByteArray(R"({"id":"thread_2"})"));
    Client bareClient(bare.baseUrl(), QStringLiteral("k"));
    const auto bareReply = awaited(bareClient.createThread());
    QVERIFY(bareReply);
    QCOMPARE(bare.requestBody(), QByteArray("{}"));
}

void TestThreadsClient::getUpdateDeleteThread()
{
    StubServer server(QList<StubServer::Response> {
            {R"({"id":"thread_1","metadata":{"user":"u1"}})"},
            {R"({"id":"thread_1","metadata":{"user":"u2"}})"},
            {R"({"id":"thread_1","object":"thread.deleted","deleted":true})"},
    });
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto fetched = awaited(client.getThread(QStringLiteral("thread_1")));
    QVERIFY(fetched);
    QVERIFY(server.requestLines().at(0).startsWith("GET /v1/threads/thread_1 "));
    QCOMPARE(fetched->thread().metadata().value(QStringLiteral("user")).toString(),
             QStringLiteral("u1"));

    const auto updated = awaited(
            client.updateThread(QStringLiteral("thread_1"),
                                QJsonObject {{QStringLiteral("user"), QStringLiteral("u2")}}));
    QVERIFY(updated);
    QVERIFY(server.requestLines().at(1).startsWith("POST /v1/threads/thread_1 "));
    QVERIFY(server.requestBodies().at(1).contains("\"metadata\":{\"user\":\"u2\"}"));

    const auto removed = awaited(client.deleteThread(QStringLiteral("thread_1")));
    QVERIFY(removed);
    QVERIFY(server.requestLines().at(2).startsWith("DELETE /v1/threads/thread_1 "));
    QCOMPARE(removed->thread().object(), QStringLiteral("thread.deleted"));
}

void TestThreadsClient::createsMessage()
{
    StubServer server(QByteArray(R"({"id":"msg_1","object":"thread.message","role":"user",)"
                                 R"("thread_id":"thread_1",)"
                                 R"("content":[{"type":"text","text":{"value":"Hello"}}]})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ThreadMessageInput message;
    message.text = QStringLiteral("Hello");

    const auto reply = awaited(client.createThreadMessage(QStringLiteral("thread_1"), message));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/threads/thread_1/messages "));
    QVERIFY(server.requestBody().contains("\"role\":\"user\""));
    QCOMPARE(reply->message().text(), QStringLiteral("Hello"));
}

void TestThreadsClient::listsMessagesFilteredByRun()
{
    StubServer server(QByteArray(R"({"object":"list","data":[)"
                                 R"({"id":"msg_1","role":"user"},)"
                                 R"({"id":"msg_2","role":"assistant",)"
                                 R"("content":[{"type":"text","text":{"value":"Sunny"}}]}],)"
                                 R"("has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.limit = 20;
    const auto reply = awaited(
            client.listThreadMessages(QStringLiteral("thread_1"), params, QStringLiteral("run_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/threads/thread_1/messages?"));
    QVERIFY(server.requestLine().contains("limit=20"));
    QVERIFY(server.requestLine().contains("run_id=run_1"));
    QCOMPARE(reply->list().size(), 2);
    QCOMPARE(reply->list().data.last().text(), QStringLiteral("Sunny"));
}

void TestThreadsClient::getUpdateDeleteMessage()
{
    StubServer server(QList<StubServer::Response> {
            {R"({"id":"msg_1","status":"completed"})"},
            {R"({"id":"msg_1","metadata":{"k":"v"}})"},
            {R"({"id":"msg_1","object":"thread.message.deleted","deleted":true})"},
    });
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto fetched
            = awaited(client.getThreadMessage(QStringLiteral("thread_1"), QStringLiteral("msg_1")));
    QVERIFY(fetched);
    QVERIFY(server.requestLines().at(0).startsWith("GET /v1/threads/thread_1/messages/msg_1 "));
    QCOMPARE(fetched->message().status(), QStringLiteral("completed"));

    const auto updated = awaited(
            client.updateThreadMessage(QStringLiteral("thread_1"), QStringLiteral("msg_1"),
                                       QJsonObject {{QStringLiteral("k"), QStringLiteral("v")}}));
    QVERIFY(updated);
    QVERIFY(server.requestLines().at(1).startsWith("POST /v1/threads/thread_1/messages/msg_1 "));

    const auto removed = awaited(
            client.deleteThreadMessage(QStringLiteral("thread_1"), QStringLiteral("msg_1")));
    QVERIFY(removed);
    QVERIFY(server.requestLines().at(2).startsWith("DELETE /v1/threads/thread_1/messages/msg_1 "));
    QCOMPARE(removed->message().object(), QStringLiteral("thread.message.deleted"));
}

void TestThreadsClient::createsRunBelowThread()
{
    StubServer server(QByteArray(R"({"id":"run_1","object":"thread.run",)"
                                 R"("thread_id":"thread_1","assistant_id":"asst_1",)"
                                 R"("status":"queued"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateRunRequest request(QStringLiteral("asst_1"));
    request.setAdditionalInstructions(QStringLiteral("Be brief."));

    const auto reply = awaited(client.createRun(QStringLiteral("thread_1"), request));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/threads/thread_1/runs "));
    QVERIFY(server.requestBody().contains("\"assistant_id\":\"asst_1\""));
    QVERIFY(server.requestBody().contains("\"additional_instructions\":\"Be brief.\""));
    QCOMPARE(reply->run().status(), RunStatus::Queued);
    QVERIFY(!reply->run().isTerminal());
}

void TestThreadsClient::createsThreadAndRunInOneCall()
{
    StubServer server(QByteArray(R"({"id":"run_1","thread_id":"thread_9","status":"queued"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateThreadRequest thread;
    thread.addUserMessage(QStringLiteral("Hello"));
    CreateRunRequest request(QStringLiteral("asst_1"));
    request.setThread(thread);

    const auto reply = awaited(client.createThreadAndRun(request));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    // The one path that is not nested below a thread id.
    QVERIFY(server.requestLine().startsWith("POST /v1/threads/runs "));
    QVERIFY(server.requestBody().contains("\"thread\":{\"messages\":"));
    QCOMPARE(reply->run().threadId(), QStringLiteral("thread_9"));
}

void TestThreadsClient::listsRuns()
{
    StubServer server(QByteArray(R"({"object":"list","data":[{"id":"run_1"},{"id":"run_2"}],)"
                                 R"("has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.limit = 5;
    const auto reply = awaited(client.listRuns(QStringLiteral("thread_1"), params));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/threads/thread_1/runs?"));
    QVERIFY(server.requestLine().contains("limit=5"));
    QCOMPARE(reply->list().size(), 2);
}

void TestThreadsClient::cancelPostsToCancelSubPath()
{
    StubServer server(QByteArray(R"({"id":"run_1","status":"cancelling"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply
            = awaited(client.cancelRun(QStringLiteral("thread_1"), QStringLiteral("run_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/threads/thread_1/runs/run_1/cancel "));
    // Cancelling is not instant: the run drains through `cancelling` first.
    QCOMPARE(reply->run().status(), RunStatus::Cancelling);
    QVERIFY(!reply->run().isTerminal());
}

void TestThreadsClient::toolLoopRunsToCompletion()
{
    // The flow the whole beta exists for: run -> requires_action -> submit the
    // outputs -> completed.
    StubServer server(QList<StubServer::Response> {
            {R"({"id":"run_1","thread_id":"thread_1","status":"requires_action",)"
             R"("required_action":{"type":"submit_tool_outputs","submit_tool_outputs":{)"
             R"("tool_calls":[{"id":"call_1","type":"function","function":{)"
             R"("name":"get_weather","arguments":"{\"city\":\"Oslo\"}"}}]}}})"},
            {R"({"id":"run_1","thread_id":"thread_1","status":"completed",)"
             R"("usage":{"prompt_tokens":10,"completion_tokens":5,"total_tokens":15}})"},
    });
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateRunRequest request(QStringLiteral("asst_1"));
    const auto started = awaited(client.createRun(QStringLiteral("thread_1"), request));
    QVERIFY(started);
    QVERIFY(started->run().requiresAction());

    // The parked calls arrive as ordinary ToolCalls, so a ToolRegistry can
    // answer them without translation.
    const QList<ToolCall> calls = started->run().requiredToolCalls();
    QCOMPARE(calls.size(), 1);
    QCOMPARE(calls.first().function().name(), QStringLiteral("get_weather"));

    QList<ToolOutput> outputs;
    outputs.append({calls.first().id(), QStringLiteral("{\"temp_c\":17}")});

    const auto submitted = awaited(
            client.submitToolOutputs(QStringLiteral("thread_1"), QStringLiteral("run_1"), outputs));
    QVERIFY(submitted);
    QVERIFY(submitted->isSuccess());
    QVERIFY(server.requestLines().at(1).startsWith(
            "POST /v1/threads/thread_1/runs/run_1/submit_tool_outputs "));
    QVERIFY(server.requestBodies().at(1).contains("\"tool_call_id\":\"call_1\""));
    QVERIFY(server.requestBodies().at(1).contains("\"output\":\"{\\\"temp_c\\\":17}\""));
    QCOMPARE(submitted->run().status(), RunStatus::Completed);
    QVERIFY(submitted->run().isTerminal());
    QCOMPARE(submitted->run().usage().totalTokens(), 15);
}

void TestThreadsClient::listsAndGetsRunSteps()
{
    StubServer server(QList<StubServer::Response> {
            {R"({"object":"list","data":[{"id":"step_1","type":"tool_calls",)"
             R"("status":"completed"}],"has_more":false})"},
            {R"({"id":"step_1","type":"message_creation","status":"completed",)"
             R"("step_details":{"type":"message_creation","message_creation":{)"
             R"("message_id":"msg_2"}}})"},
    });
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto list
            = awaited(client.listRunSteps(QStringLiteral("thread_1"), QStringLiteral("run_1")));
    QVERIFY(list);
    QVERIFY(list->isSuccess());
    QVERIFY(server.requestLines().at(0).startsWith("GET /v1/threads/thread_1/runs/run_1/steps"));
    QCOMPARE(list->list().size(), 1);
    QCOMPARE(list->list().data.first().status(), RunStepStatus::Completed);

    const auto step = awaited(client.getRunStep(QStringLiteral("thread_1"), QStringLiteral("run_1"),
                                                QStringLiteral("step_1")));
    QVERIFY(step);
    QVERIFY(server.requestLines().at(1).startsWith(
            "GET /v1/threads/thread_1/runs/run_1/steps/step_1 "));
    QCOMPARE(step->step()
                     .stepDetails()
                     .value(QStringLiteral("message_creation"))
                     .toObject()
                     .value(QStringLiteral("message_id"))
                     .toString(),
             QStringLiteral("msg_2"));
}

void TestThreadsClient::pollerStopsOnRequiredAction()
{
    // A parked run never moves on its own, so the poller has to stop for it --
    // with its own signal, because the caller has work to do.
    StubServer server(QList<StubServer::Response> {
            {R"({"id":"run_1","status":"queued"})"},
            {R"({"id":"run_1","status":"requires_action","required_action":{)"
             R"("type":"submit_tool_outputs","submit_tool_outputs":{"tool_calls":[)"
             R"({"id":"call_1","function":{"name":"get_weather","arguments":"{}"}}]}}})"},
    });
    Client client(server.baseUrl(), QStringLiteral("k"));

    RunPoller *poller = client.pollRun(QStringLiteral("thread_1"), QStringLiteral("run_1"), 10);
    poller->setAutoDelete(false);

    QSignalSpy actionSpy(poller, &RunPoller::requiresAction);
    QSignalSpy completedSpy(poller, &RunPoller::completed);

    poller->start();
    QVERIFY(actionSpy.wait(5000));

    QCOMPARE(actionSpy.count(), 1);
    QCOMPARE(completedSpy.count(), 0);
    QVERIFY(poller->isFinished());
    QCOMPARE(poller->run().requiredToolCalls().size(), 1);
    QCOMPARE(poller->threadId(), QStringLiteral("thread_1"));
    QCOMPARE(poller->runId(), QStringLiteral("run_1"));
    delete poller;
}

void TestThreadsClient::pollerFollowsRunToCompletion()
{
    StubServer server(QList<StubServer::Response> {
            {R"({"id":"run_1","status":"queued"})"},
            {R"({"id":"run_1","status":"in_progress"})"},
            {R"({"id":"run_1","status":"completed"})"},
    });
    Client client(server.baseUrl(), QStringLiteral("k"));

    RunPoller *poller = client.pollRun(QStringLiteral("thread_1"), QStringLiteral("run_1"), 10);
    poller->setAutoDelete(false);

    QList<RunStatus> observed;
    connect(poller, &RunPoller::progressed, this,
            [&observed](const Run &run) { observed.append(run.status()); });
    QSignalSpy completedSpy(poller, &RunPoller::completed);

    poller->start();
    QVERIFY(completedSpy.wait(5000));

    QCOMPARE(completedSpy.count(), 1);
    QVERIFY(poller->isFinished());
    QCOMPARE(poller->run().status(), RunStatus::Completed);
    QCOMPARE(observed,
             QList<RunStatus>({RunStatus::Queued, RunStatus::InProgress, RunStatus::Completed}));
    QVERIFY(server.requestLines().first().startsWith("GET /v1/threads/thread_1/runs/run_1 "));
    delete poller;
}

void TestThreadsClient::streamsMessageDeltasAndRunStates()
{
    // Assistants events name their type in the SSE `event:` field, but every
    // payload is a whole object that names itself -- which is what the reply
    // routes on.
    const QByteArray sse
            = "event: thread.run.created\n"
              "data: {\"id\":\"run_1\",\"object\":\"thread.run\",\"status\":\"queued\"}\n\n"
              "event: thread.message.delta\n"
              "data: {\"id\":\"msg_1\",\"object\":\"thread.message.delta\",\"delta\":{"
              "\"content\":[{\"index\":0,\"type\":\"text\",\"text\":{\"value\":\"Sun\"}}]}}\n\n"
              "event: thread.message.delta\n"
              "data: {\"id\":\"msg_1\",\"object\":\"thread.message.delta\",\"delta\":{"
              "\"content\":[{\"index\":0,\"type\":\"text\",\"text\":{\"value\":\"ny\"}}]}}\n\n"
              "event: thread.run.completed\n"
              "data: {\"id\":\"run_1\",\"object\":\"thread.run\",\"status\":\"completed\"}\n\n"
              "event: done\n"
              "data: [DONE]\n\n";
    RunSseStubServer server(sse);
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateRunRequest request(QStringLiteral("asst_1"));
    RunStreamReply *reply = client.createRunStream(QStringLiteral("thread_1"), request);
    reply->setAutoDelete(false);

    QString text;
    connect(reply, &RunStreamReply::messageDelta, this,
            [&text](const QString &delta) { text += delta; });
    QList<RunStatus> states;
    connect(reply, &RunStreamReply::runChanged, this,
            [&states](const Run &run) { states.append(run.status()); });
    QSignalSpy finishedSpy(reply, &RunStreamReply::finished);

    QVERIFY(finishedSpy.wait(5000));

    QVERIFY(reply->isSuccess());
    QCOMPARE(text, QStringLiteral("Sunny"));
    QCOMPARE(states, QList<RunStatus>({RunStatus::Queued, RunStatus::Completed}));
    QCOMPARE(reply->run().status(), RunStatus::Completed);
    // The streamed request must announce both the stream and the beta.
    const QByteArray head = server.requestHead().toLower();
    QVERIFY(head.contains("openai-beta: assistants=v2"));
    QVERIFY(head.contains("accept: text/event-stream"));
    delete reply;
}

void TestThreadsClient::streamReportsOnlySettledMessages()
{
    // Every message lifecycle event carries the same bare message object -- only
    // the SSE `event:` name says which of them arrived, so routing on the
    // payload alone would report one finished message three times.
    const QByteArray sse
            = "event: thread.message.created\n"
              "data: {\"id\":\"msg_1\",\"object\":\"thread.message\","
              "\"status\":\"in_progress\",\"content\":[]}\n\n"
              "event: thread.message.in_progress\n"
              "data: {\"id\":\"msg_1\",\"object\":\"thread.message\","
              "\"status\":\"in_progress\",\"content\":[]}\n\n"
              "event: thread.message.delta\n"
              "data: {\"id\":\"msg_1\",\"object\":\"thread.message.delta\",\"delta\":{"
              "\"content\":[{\"index\":0,\"type\":\"text\",\"text\":{\"value\":"
              "\"Sunny\"}}]}}\n\n"
              "event: thread.message.completed\n"
              "data: {\"id\":\"msg_1\",\"object\":\"thread.message\","
              "\"status\":\"completed\",\"content\":[{\"type\":\"text\",\"text\":{"
              "\"value\":\"Sunny\"}}]}\n\n"
              "event: thread.run.completed\n"
              "data: {\"id\":\"run_1\",\"object\":\"thread.run\","
              "\"status\":\"completed\"}\n\n"
              "event: done\ndata: [DONE]\n\n";
    RunSseStubServer server(sse);
    Client client(server.baseUrl(), QStringLiteral("k"));

    RunStreamReply *reply
            = client.createRunStream(QStringLiteral("thread_1"),
                                     CreateRunRequest(QStringLiteral("asst_1")));
    reply->setAutoDelete(false);

    QList<ThreadMessage> completed;
    connect(reply, &RunStreamReply::messageCompleted, this,
            [&completed](const ThreadMessage &message) { completed.append(message); });
    QStringList eventTypes;
    connect(reply, &RunStreamReply::event, this,
            [&eventTypes](const QString &type, const QJsonObject &) { eventTypes.append(type); });
    QSignalSpy finishedSpy(reply, &RunStreamReply::finished);

    QVERIFY(finishedSpy.wait(5000));

    QCOMPARE(completed.size(), 1);
    QCOMPARE(completed.first().status(), QStringLiteral("completed"));
    QCOMPARE(completed.first().text(), QStringLiteral("Sunny"));
    // The event signal reports the name the API used, not the payload's object.
    QVERIFY(eventTypes.contains(QStringLiteral("thread.message.created")));
    QVERIFY(eventTypes.contains(QStringLiteral("thread.message.completed")));
    delete reply;
}

void TestThreadsClient::streamSurfacesErrorEvents()
{
    // The error event is a bare ErrorObject -- no `error` wrapper, no `object`.
    const QByteArray sse = "event: thread.run.created\n"
                           "data: {\"id\":\"run_1\",\"object\":\"thread.run\","
                           "\"status\":\"queued\"}\n\n"
                           "event: error\n"
                           "data: {\"message\":\"The server had an error\","
                           "\"type\":\"server_error\",\"code\":\"internal\"}\n\n";
    RunSseStubServer server(sse);
    Client client(server.baseUrl(), QStringLiteral("k"));

    RunStreamReply *reply
            = client.createRunStream(QStringLiteral("thread_1"),
                                     CreateRunRequest(QStringLiteral("asst_1")));
    reply->setAutoDelete(false);

    QSignalSpy failedSpy(reply, &RunStreamReply::failed);
    QVERIFY(failedSpy.wait(5000));

    QVERIFY(!reply->isSuccess());
    // The provider's own message survives instead of a generic parse error.
    QCOMPARE(reply->error().kind(), ClientError::Kind::Http);
    QCOMPARE(reply->error().message(), QStringLiteral("The server had an error"));
    QCOMPARE(reply->error().type(), QStringLiteral("server_error"));
    QCOMPARE(reply->error().code(), QStringLiteral("internal"));
    delete reply;
}

void TestThreadsClient::streamedToolOutputsResumeTheRun()
{
    // A streamed run that parks on requires_action is resumed as a new stream,
    // so the tool loop never has to fall back to polling.
    const QByteArray sse = "event: thread.run.in_progress\n"
                           "data: {\"id\":\"run_1\",\"object\":\"thread.run\","
                           "\"status\":\"in_progress\"}\n\n"
                           "event: thread.run.completed\n"
                           "data: {\"id\":\"run_1\",\"object\":\"thread.run\","
                           "\"status\":\"completed\"}\n\n"
                           "event: done\ndata: [DONE]\n\n";
    RunSseStubServer server(sse);
    Client client(server.baseUrl(), QStringLiteral("k"));

    QList<ToolOutput> outputs;
    outputs.append({QStringLiteral("call_1"), QStringLiteral("{\"temp_c\":17}")});

    RunStreamReply *reply = client.submitToolOutputsStream(QStringLiteral("thread_1"),
                                                            QStringLiteral("run_1"), outputs);
    reply->setAutoDelete(false);

    QSignalSpy finishedSpy(reply, &RunStreamReply::finished);
    QVERIFY(finishedSpy.wait(5000));

    QVERIFY(reply->isSuccess());
    QCOMPARE(reply->run().status(), RunStatus::Completed);
    const QByteArray head = server.requestHead();
    QVERIFY(head.startsWith("POST /v1/threads/thread_1/runs/run_1/submit_tool_outputs "));
    QVERIFY(head.toLower().contains("openai-beta: assistants=v2"));
    QVERIFY(server.requestBody().contains("\"stream\":true"));
    QVERIFY(server.requestBody().contains("\"tool_call_id\":\"call_1\""));
    delete reply;
}

void TestThreadsClient::updateRunPostsMetadata()
{
    StubServer server(QByteArray(R"({"id":"run_1","metadata":{"k":"v"}})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.updateRun(QStringLiteral("thread_1"),
                                                QStringLiteral("run_1"),
                                                QJsonObject {{QStringLiteral("k"),
                                                              QStringLiteral("v")}}));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/threads/thread_1/runs/run_1 "));
    QCOMPARE(server.requestBody(), QByteArray(R"({"metadata":{"k":"v"}})"));
    QCOMPARE(reply->run().metadata().value(QStringLiteral("k")).toString(), QStringLiteral("v"));
}

QTEST_MAIN(TestThreadsClient)
#include "tst_threads.moc"
