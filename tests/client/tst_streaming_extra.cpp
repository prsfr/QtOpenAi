// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

// A throwaway HTTP/1.1 server that streams a canned Server-Sent-Events body and
// closes, exercising the SSE parser offline.
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

// Wrap a JSON payload as one SSE event.
static QByteArray sse(const QByteArray &json) { return "data: " + json + "\n\n"; }

class TestStreamingExtra : public QObject
{
    Q_OBJECT
private slots:
    void legacyCompletionStream();
    void responseStreamEmitsDeltasAndFinal();
    void responseStreamFailedEvent();
};

void TestStreamingExtra::legacyCompletionStream()
{
    QByteArray body;
    body += sse(R"({"id":"cmpl_1","object":"text_completion","model":"davinci-002",)"
                R"("choices":[{"text":"Hel","index":0,"finish_reason":null}]})");
    body += sse(R"({"choices":[{"text":"lo","index":0,"finish_reason":"stop"}]})");
    body += sse("[DONE]");
    SseStubServer server(200, body);
    Client client(server.baseUrl(), QStringLiteral("k"));

    CompletionStreamReply *reply = client.createCompletionStream(
            CompletionRequest(QStringLiteral("davinci-002"), QStringLiteral("Hi")));
    reply->setAutoDelete(false);

    QStringList deltas;
    connect(reply, &CompletionStreamReply::textDelta, this,
            [&deltas](const QString &text) { deltas << text; });
    QSignalSpy finishedSpy(reply, &CompletionStreamReply::finished);

    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QCOMPARE(deltas, (QStringList {QStringLiteral("Hel"), QStringLiteral("lo")}));
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(reply->response().firstText(), QStringLiteral("Hello"));
    QCOMPARE(reply->response().choices().first().finishReason(), QStringLiteral("stop"));
    delete reply;
}

void TestStreamingExtra::responseStreamEmitsDeltasAndFinal()
{
    QByteArray body;
    body += sse(R"({"type":"response.created","response":{"id":"resp_1"}})");
    body += sse(R"({"type":"response.output_text.delta","delta":"Hello "})");
    body += sse(R"({"type":"response.output_text.delta","delta":"world"})");
    body += sse(R"({"type":"response.completed","response":{"id":"resp_1",)"
                R"("object":"response","status":"completed","output":[{"type":"message",)"
                R"("role":"assistant","content":[{"type":"output_text","text":"Hello world",)"
                R"("annotations":[]}]}]}})");
    SseStubServer server(200, body);
    Client client(server.baseUrl(), QStringLiteral("k"));

    ResponseStreamReply *reply = client.createResponseStream(
            ResponseRequest(QStringLiteral("gpt-5"), QStringLiteral("hi")));
    reply->setAutoDelete(false);

    QStringList deltas;
    connect(reply, &ResponseStreamReply::outputTextDelta, this,
            [&deltas](const QString &text) { deltas << text; });
    QSignalSpy eventSpy(reply, &ResponseStreamReply::event);
    QSignalSpy finishedSpy(reply, &ResponseStreamReply::finished);

    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QCOMPARE(deltas, (QStringList {QStringLiteral("Hello "), QStringLiteral("world")}));
    QCOMPARE(eventSpy.count(), 4);
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(reply->response().status(), QStringLiteral("completed"));
    QCOMPARE(reply->response().outputText(), QStringLiteral("Hello world"));
    delete reply;
}

void TestStreamingExtra::responseStreamFailedEvent()
{
    QByteArray body;
    body += sse(R"({"type":"response.created","response":{"id":"resp_1"}})");
    body += sse(R"({"type":"response.failed","response":{"error":{"message":"boom",)"
                R"("type":"server_error"}}})");
    SseStubServer server(200, body);
    Client client(server.baseUrl(), QStringLiteral("k"));

    ResponseStreamReply *reply = client.createResponseStream(
            ResponseRequest(QStringLiteral("gpt-5"), QStringLiteral("hi")));
    reply->setAutoDelete(false);
    QSignalSpy failedSpy(reply, &ResponseStreamReply::failed);

    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(!reply->isSuccess());
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(reply->error().message(), QStringLiteral("boom"));
    delete reply;
}

QTEST_MAIN(TestStreamingExtra)
#include "tst_streaming_extra.moc"
