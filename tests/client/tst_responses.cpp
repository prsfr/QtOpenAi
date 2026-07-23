// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

// A one-shot stub server that returns a fixed body and records the request's
// method + path (its first request line) so the client's routing can be checked.
class StubServer : public QObject
{
    Q_OBJECT
public:
    explicit StubServer(int status, QByteArray body, QObject *parent = nullptr)
        : QObject(parent)
        , m_status(status)
        , m_body(std::move(body))
    {
        m_server.listen(QHostAddress::LocalHost, 0);
        connect(&m_server, &QTcpServer::newConnection, this, &StubServer::onConnection);
    }

    QUrl baseUrl() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1/v1").arg(m_server.serverPort()));
    }

    QByteArray requestLine() const { return m_requestLine; }

private slots:
    void onConnection()
    {
        QTcpSocket *socket = m_server.nextPendingConnection();
        auto buffer = std::make_shared<QByteArray>();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer]() {
            *buffer += socket->readAll();
            if (!buffer->contains("\r\n\r\n"))
                return;
            if (m_requestLine.isEmpty())
                m_requestLine = buffer->left(buffer->indexOf("\r\n"));

            QByteArray response = "HTTP/1.1 " + QByteArray::number(m_status)
                                  + " X\r\n"
                                    "Content-Type: application/json\r\n"
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
    QByteArray m_requestLine;
};

class TestResponsesClient : public QObject
{
    Q_OBJECT
private:
    static QByteArray responseBody(const char *status = "completed")
    {
        // Single balanced raw string (a placeholder keeps moc's brace counter
        // happy) with the status substituted in.
        QByteArray body = R"({"id":"resp_1","object":"response","created_at":1,"model":"gpt-5",
            "status":"__STATUS__","output":[{"type":"message","id":"msg_1","status":"completed",
            "role":"assistant","content":[{"type":"output_text","text":"hi","annotations":[]}]}],
            "usage":{"input_tokens":1,"output_tokens":1,"total_tokens":2,
            "output_tokens_details":{"reasoning_tokens":0}}})";
        body.replace("__STATUS__", status);
        return body;
    }

private slots:
    void createResponsePostsAndParses();
    void getResponseUsesGet();
    void cancelResponsePostsToCancel();
    void deleteResponseUsesDelete();
    void httpErrorSurfacesMessage();
};

void TestResponsesClient::createResponsePostsAndParses()
{
    StubServer server(200, responseBody());
    Client client(server.baseUrl(), QStringLiteral("k"));

    ResponseReply *reply
            = client.createResponse(ResponseRequest(QStringLiteral("gpt-5"), QStringLiteral("hi")));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/responses "));
    QCOMPARE(reply->response().id(), QStringLiteral("resp_1"));
    QCOMPARE(reply->response().outputText(), QStringLiteral("hi"));
    delete reply;
}

void TestResponsesClient::getResponseUsesGet()
{
    StubServer server(200, responseBody());
    Client client(server.baseUrl(), QStringLiteral("k"));

    ResponseReply *reply = client.getResponse(QStringLiteral("resp_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/responses/resp_1 "));
    delete reply;
}

void TestResponsesClient::cancelResponsePostsToCancel()
{
    StubServer server(200, responseBody("cancelled"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ResponseReply *reply = client.cancelResponse(QStringLiteral("resp_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/responses/resp_1/cancel "));
    QCOMPARE(reply->response().status(), QStringLiteral("cancelled"));
    delete reply;
}

void TestResponsesClient::deleteResponseUsesDelete()
{
    StubServer server(200, R"({"id":"resp_1","object":"response.deleted","deleted":true})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    ResponseReply *reply = client.deleteResponse(QStringLiteral("resp_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("DELETE /v1/responses/resp_1 "));
    QCOMPARE(reply->response().object(), QStringLiteral("response.deleted"));
    delete reply;
}

void TestResponsesClient::httpErrorSurfacesMessage()
{
    StubServer server(404, R"({"error":{"message":"No response found","type":"invalid_request"}})");
    Client client(server.baseUrl(), QStringLiteral("k"));
    client.setRetryPolicy(RetryPolicy::none());

    ResponseReply *reply = client.getResponse(QStringLiteral("missing"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(!reply->isSuccess());
    QCOMPARE(reply->error().httpStatus(), 404);
    QCOMPARE(reply->error().message(), QStringLiteral("No response found"));
    delete reply;
}

QTEST_MAIN(TestResponsesClient)
#include "tst_responses.moc"
