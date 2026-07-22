// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/ChatCompletionAccumulator.h>
#include <QtOpenAi/Client/Client.h>

#include <QtCore/QJsonDocument>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

namespace {

ChatCompletionChunk chunkFromJson(const QByteArray &json)
{
    return ChatCompletionChunk::fromJson(QJsonDocument::fromJson(json).object());
}

} // namespace

// A throwaway HTTP/1.1 server that streams a canned Server-Sent-Events body
// (or a plain JSON error body) and closes, exercising the SSE parser offline.
class SseStubServer : public QObject
{
    Q_OBJECT
public:
    SseStubServer(int status, QByteArray body, QObject *parent = nullptr)
        : QObject(parent)
        , m_status(status)
        , m_body(std::move(body))
    {
        m_server.listen(QHostAddress::LocalHost, 0);
        connect(&m_server, &QTcpServer::newConnection, this, &SseStubServer::onConnection);
    }

    QUrl baseUrl() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1/v1").arg(m_server.serverPort()));
    }

private slots:
    void onConnection()
    {
        QTcpSocket *socket = m_server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            m_request += socket->readAll();
            if (!m_request.contains("\r\n\r\n"))
                return;

            const QByteArray contentType
                    = m_status < 400 ? "text/event-stream" : "application/json";
            QByteArray response = "HTTP/1.1 " + QByteArray::number(m_status)
                                  + " OK\r\n"
                                    "Content-Type: "
                                  + contentType
                                  + "\r\n"
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
    int m_status;
    QByteArray m_body;
    QByteArray m_request;
};

class TestStreaming : public QObject
{
    Q_OBJECT
private slots:
    void accumulatorConcatenatesContent();
    void accumulatorReassemblesToolCalls();
    void streamEmitsOrderedContentDeltasAndFinal();
    void streamReassemblesToolCalls();
    void streamHttpErrorEmitsFailed();
};

void TestStreaming::accumulatorConcatenatesContent()
{
    ChatCompletionAccumulator acc;
    acc.add(chunkFromJson(R"({"id":"c1","model":"gpt-4o",
        "choices":[{"index":0,"delta":{"role":"assistant","content":"Hel"}}]})"));
    acc.add(chunkFromJson(R"({"choices":[{"index":0,"delta":{"content":"lo"},
        "finish_reason":"stop"}]})"));

    QCOMPARE(acc.content(), QStringLiteral("Hello"));

    const ChatCompletionResponse response = acc.response();
    QCOMPARE(response.id(), QStringLiteral("c1"));
    QCOMPARE(response.model(), QStringLiteral("gpt-4o"));
    QCOMPARE(response.choices().size(), 1);
    QCOMPARE(response.firstMessage().role(), Role::Assistant);
    QCOMPARE(response.firstMessage().content(), QStringLiteral("Hello"));
    QCOMPARE(response.choices().first().finishReason(), FinishReason::Stop);
}

void TestStreaming::accumulatorReassemblesToolCalls()
{
    ChatCompletionAccumulator acc;
    acc.add(chunkFromJson(R"({"choices":[{"index":0,"delta":{"tool_calls":[
        {"index":0,"id":"call_1","type":"function",
         "function":{"name":"get_weather","arguments":"{\"loc"}}]}}]})"));
    acc.add(chunkFromJson(R"({"choices":[{"index":0,"delta":{"tool_calls":[
        {"index":0,"function":{"arguments":"ation\":\"Berlin\"}"}}]},
        "finish_reason":"tool_calls"}]})"));

    const QList<ToolCall> calls = acc.response().toolCalls();
    QCOMPARE(calls.size(), 1);
    QCOMPARE(calls.first().id(), QStringLiteral("call_1"));
    QCOMPARE(calls.first().function().name(), QStringLiteral("get_weather"));
    QCOMPARE(
            calls.first().function().argumentsObject().value(QStringLiteral("location")).toString(),
            QStringLiteral("Berlin"));
    QCOMPARE(acc.response().choices().first().finishReason(), FinishReason::ToolCalls);
}

void TestStreaming::streamEmitsOrderedContentDeltasAndFinal()
{
    const QByteArray sse
            = "data: {\"id\":\"c1\",\"model\":\"gpt-4o\",\"choices\":[{\"index\":0,"
              "\"delta\":{\"role\":\"assistant\",\"content\":\"Hel\"}}]}\n\n"
              "data: {\"choices\":[{\"index\":0,\"delta\":{\"content\":\"lo\"}}]}\n\n"
              "data: {\"choices\":[{\"index\":0,\"delta\":{},\"finish_reason\":\"stop\"}]}\n\n"
              "data: [DONE]\n\n";
    SseStubServer server(200, sse);
    Client client(server.baseUrl(), QStringLiteral("k"));

    ChatCompletionStreamReply *reply = client.createChatCompletionStream(
            ChatCompletionRequest(QStringLiteral("gpt-4o"), {Message::user(QStringLiteral("hi"))}));
    reply->setAutoDelete(false);

    QStringList deltas;
    connect(reply, &ChatCompletionStreamReply::contentDelta, this,
            [&deltas](const QString &text) { deltas << text; });
    QSignalSpy finishedSpy(reply, &ChatCompletionStreamReply::finished);

    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QCOMPARE(deltas, (QStringList {QStringLiteral("Hel"), QStringLiteral("lo")}));
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(reply->response().firstMessage().content(), QStringLiteral("Hello"));
    QCOMPARE(reply->response().choices().first().finishReason(), FinishReason::Stop);
    delete reply;
}

void TestStreaming::streamReassemblesToolCalls()
{
    const QByteArray sse
            = "data: {\"choices\":[{\"index\":0,\"delta\":{\"tool_calls\":[{\"index\":0,"
              "\"id\":\"call_1\",\"type\":\"function\",\"function\":{\"name\":\"get_weather\","
              "\"arguments\":\"{\\\"location\\\":\"}}]}}]}\n\n"
              "data: {\"choices\":[{\"index\":0,\"delta\":{\"tool_calls\":[{\"index\":0,"
              "\"function\":{\"arguments\":\"\\\"Paris\\\"}\"}}]},\"finish_reason\":\"tool_calls\"}"
              "]}\n\n"
              "data: [DONE]\n\n";
    SseStubServer server(200, sse);
    Client client(server.baseUrl(), QStringLiteral("k"));

    ChatCompletionStreamReply *reply = client.createChatCompletionStream(
            ChatCompletionRequest(QStringLiteral("gpt-4o"), {Message::user(QStringLiteral("hi"))}));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    const QList<ToolCall> calls = reply->response().toolCalls();
    QCOMPARE(calls.size(), 1);
    QCOMPARE(calls.first().function().name(), QStringLiteral("get_weather"));
    QCOMPARE(
            calls.first().function().argumentsObject().value(QStringLiteral("location")).toString(),
            QStringLiteral("Paris"));
    delete reply;
}

void TestStreaming::streamHttpErrorEmitsFailed()
{
    const QByteArray body = R"({"error":{"message":"boom","type":"server_error","code":"x"}})";
    SseStubServer server(500, body);
    Client client(server.baseUrl(), QStringLiteral("k"));

    ChatCompletionStreamReply *reply = client.createChatCompletionStream(
            ChatCompletionRequest(QStringLiteral("gpt-4o"), {Message::user(QStringLiteral("hi"))}));
    reply->setAutoDelete(false);

    QSignalSpy failedSpy(reply, &ChatCompletionStreamReply::failed);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(!reply->isSuccess());
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(reply->error().kind(), ClientError::Kind::Http);
    QCOMPARE(reply->error().httpStatus(), 500);
    QCOMPARE(reply->error().message(), QStringLiteral("boom"));
    delete reply;
}

QTEST_MAIN(TestStreaming)
#include "tst_streaming.moc"
