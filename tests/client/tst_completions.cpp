// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

// A one-shot stub server: returns a fixed body and records the request's
// method + path (its first request line) plus the request body.
class StubServer : public QObject
{
    Q_OBJECT
public:
    explicit StubServer(QByteArray body, QObject *parent = nullptr)
        : QObject(parent)
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
    QByteArray requestBody() const { return m_requestBody; }

private slots:
    void onConnection()
    {
        QTcpSocket *socket = m_server.nextPendingConnection();
        auto buffer = std::make_shared<QByteArray>();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer]() {
            *buffer += socket->readAll();
            if (!buffer->contains("\r\n\r\n"))
                return;
            if (m_requestLine.isEmpty()) {
                m_requestLine = buffer->left(buffer->indexOf("\r\n"));
                m_requestBody = buffer->mid(buffer->indexOf("\r\n\r\n") + 4);
            }

            QByteArray response = "HTTP/1.1 200 X\r\n"
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
    QByteArray m_body;
    QByteArray m_requestLine;
    QByteArray m_requestBody;
};

class TestCompletionsClient : public QObject
{
    Q_OBJECT
private slots:
    void createPostsAndParses();
    void networkErrorIsReported();
};

void TestCompletionsClient::createPostsAndParses()
{
    StubServer server(R"({"id":"cmpl_1","object":"text_completion","created":1,
        "model":"davinci-002","choices":[{"text":" world","index":0,"logprobs":null,
        "finish_reason":"stop"}],"usage":{"prompt_tokens":1,"completion_tokens":1,
        "total_tokens":2}})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    CompletionReply *reply = client.createCompletion(
            CompletionRequest(QStringLiteral("davinci-002"), QStringLiteral("Hello")));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/completions "));
    QVERIFY(server.requestBody().contains("\"prompt\":\"Hello\""));
    QCOMPARE(reply->response().firstText(), QStringLiteral(" world"));
    delete reply;
}

void TestCompletionsClient::networkErrorIsReported()
{
    // Point at a closed port to exercise the failure path.
    Client client(QUrl(QStringLiteral("http://127.0.0.1:1/v1")), QStringLiteral("k"));
    client.setRetryPolicy(RetryPolicy::none());

    CompletionReply *reply
            = client.createCompletion(CompletionRequest(QStringLiteral("m"), QStringLiteral("x")));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(!reply->isSuccess());
    QVERIFY(reply->error().isError());
    delete reply;
}

QTEST_MAIN(TestCompletionsClient)
#include "tst_completions.moc"
