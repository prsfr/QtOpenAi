// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtCore/QJsonDocument>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

// A throwaway HTTP/1.1 server that replies to a single request with a canned
// status line and JSON body. Lets us exercise the full Client -> reply path
// (request building, transport, response parsing) without any network access.
class StubServer : public QObject
{
    Q_OBJECT
public:
    StubServer(int status, QByteArray body, QObject *parent = nullptr)
        : QObject(parent), m_status(status), m_body(std::move(body))
    {
        m_server.listen(QHostAddress::LocalHost, 0);
        connect(&m_server, &QTcpServer::newConnection, this, &StubServer::onConnection);
    }

    QUrl baseUrl() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1/v1").arg(m_server.serverPort()));
    }

    QByteArray requestBody() const { return m_requestBody; }

private slots:
    void onConnection()
    {
        QTcpSocket *socket = m_server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            m_requestBuffer += socket->readAll();
            // Wait until headers are complete, then capture the body.
            const int headerEnd = m_requestBuffer.indexOf("\r\n\r\n");
            if (headerEnd < 0)
                return;
            m_requestBody = m_requestBuffer.mid(headerEnd + 4);

            const QByteArray reason = m_status < 400 ? "OK" : "Error";
            QByteArray response =
                "HTTP/1.1 " + QByteArray::number(m_status) + " " + reason + "\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: " + QByteArray::number(m_body.size()) + "\r\n"
                "Connection: close\r\n\r\n" + m_body;
            socket->write(response);
            socket->flush();
            socket->disconnectFromHost();
        });
    }

private:
    QTcpServer m_server;
    int m_status;
    QByteArray m_body;
    QByteArray m_requestBuffer;
    QByteArray m_requestBody;
};

class TestReply : public QObject
{
    Q_OBJECT
private slots:
    void successfulCompletionEmitsFinished();
    void httpErrorEmitsFailedWithDetails();
    void requestBodyContainsModelAndMessages();
};

void TestReply::successfulCompletionEmitsFinished()
{
    const QByteArray body = R"({
        "id": "chatcmpl-1", "object": "chat.completion", "created": 1, "model": "gpt-4o",
        "choices": [{"index": 0, "message": {"role": "assistant", "content": "Hi!"},
                     "finish_reason": "stop"}],
        "usage": {"prompt_tokens": 1, "completion_tokens": 1, "total_tokens": 2}
    })";
    StubServer server(200, body);
    Client client(server.baseUrl(), QStringLiteral("test-key"));

    ChatCompletionReply *reply = client.createChatCompletion(
        ChatCompletionRequest(QStringLiteral("gpt-4o"),
                              {Message::user(QStringLiteral("hi"))}));
    reply->setAutoDelete(false);

    QSignalSpy finishedSpy(reply, &ChatCompletionReply::finished);
    QSignalSpy failedSpy(reply, &ChatCompletionReply::failed);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(finishedSpy.count(), 1);
    QVERIFY(reply->isSuccess());
    QCOMPARE(reply->response().firstMessage().content(), QStringLiteral("Hi!"));
    delete reply;
}

void TestReply::httpErrorEmitsFailedWithDetails()
{
    const QByteArray body = R"({
        "error": {"message": "Invalid API key", "type": "invalid_request_error", "code": "invalid_api_key"}
    })";
    StubServer server(401, body);
    Client client(server.baseUrl(), QStringLiteral("bad-key"));

    ChatCompletionReply *reply = client.createChatCompletion(
        ChatCompletionRequest(QStringLiteral("gpt-4o"),
                              {Message::user(QStringLiteral("hi"))}));
    reply->setAutoDelete(false);

    QSignalSpy failedSpy(reply, &ChatCompletionReply::failed);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(!reply->isSuccess());
    QCOMPARE(failedSpy.count(), 1);
    const ClientError error = reply->error();
    QCOMPARE(error.kind(), ClientError::Kind::Http);
    QCOMPARE(error.httpStatus(), 401);
    QCOMPARE(error.message(), QStringLiteral("Invalid API key"));
    QCOMPARE(error.type(), QStringLiteral("invalid_request_error"));
    QCOMPARE(error.code(), QStringLiteral("invalid_api_key"));
    delete reply;
}

void TestReply::requestBodyContainsModelAndMessages()
{
    const QByteArray body = R"({
        "id": "x", "object": "chat.completion", "created": 1, "model": "gpt-4o",
        "choices": [], "usage": {"prompt_tokens": 0, "completion_tokens": 0, "total_tokens": 0}
    })";
    StubServer server(200, body);
    Client client(server.baseUrl(), QStringLiteral("k"));

    ChatCompletionReply *reply = client.createChatCompletion(
        ChatCompletionRequest(QStringLiteral("gpt-4o-mini"),
                              {Message::user(QStringLiteral("ping"))}));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    const QJsonObject sent = QJsonDocument::fromJson(server.requestBody()).object();
    QCOMPARE(sent.value(QStringLiteral("model")).toString(), QStringLiteral("gpt-4o-mini"));
    QCOMPARE(sent.value(QStringLiteral("messages")).toArray().size(), 1);
    delete reply;
}

QTEST_MAIN(TestReply)
#include "tst_reply.moc"
