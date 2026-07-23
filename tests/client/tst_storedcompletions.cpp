// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

// A one-shot stub server: returns a fixed body and records the request's
// method + path (its first request line) so routing can be asserted.
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
};

class TestStoredCompletions : public QObject
{
    Q_OBJECT
private:
    static QByteArray completionBody()
    {
        return R"({"id":"cmpl_1","object":"chat.completion","created":1,"model":"gpt-4o",
            "choices":[{"index":0,"message":{"role":"assistant","content":"ok"},
            "finish_reason":"stop"}],"usage":{"prompt_tokens":1,"completion_tokens":1,
            "total_tokens":2}})";
    }

private slots:
    void getUsesGetAndParses();
    void listUsesGetWithPagination();
    void deleteUsesDelete();
    void listMessagesUsesGet();
};

void TestStoredCompletions::getUsesGetAndParses()
{
    StubServer server(completionBody());
    Client client(server.baseUrl(), QStringLiteral("k"));

    ChatCompletionReply *reply = client.getChatCompletion(QStringLiteral("cmpl_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/chat/completions/cmpl_1 "));
    QCOMPARE(reply->response().id(), QStringLiteral("cmpl_1"));
    delete reply;
}

void TestStoredCompletions::listUsesGetWithPagination()
{
    StubServer server(R"({"object":"list","data":[
        {"id":"cmpl_1","object":"chat.completion","created":1,"model":"gpt-4o",
         "choices":[],"usage":{"prompt_tokens":1,"completion_tokens":1,"total_tokens":2}}],
        "first_id":"cmpl_1","last_id":"cmpl_1","has_more":true})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.limit = 5;
    params.after = QStringLiteral("cmpl_0");
    ChatCompletionListReply *reply = client.listChatCompletions(params);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/chat/completions?"));
    QVERIFY(server.requestLine().contains("limit=5"));
    QVERIFY(server.requestLine().contains("after=cmpl_0"));
    QCOMPARE(reply->list().size(), 1);
    QVERIFY(reply->list().hasMore);
    delete reply;
}

void TestStoredCompletions::deleteUsesDelete()
{
    StubServer server(R"({"id":"cmpl_1","object":"chat.completion.deleted","deleted":true})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    ChatCompletionReply *reply = client.deleteChatCompletion(QStringLiteral("cmpl_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("DELETE /v1/chat/completions/cmpl_1 "));
    QCOMPARE(reply->response().object(), QStringLiteral("chat.completion.deleted"));
    delete reply;
}

void TestStoredCompletions::listMessagesUsesGet()
{
    StubServer server(R"({"object":"list","data":[
        {"role":"user","content":"hi"}],
        "first_id":"msg_1","last_id":"msg_1","has_more":false})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    ChatCompletionMessageListReply *reply
            = client.listChatCompletionMessages(QStringLiteral("cmpl_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/chat/completions/cmpl_1/messages "));
    QCOMPARE(reply->list().size(), 1);
    delete reply;
}

QTEST_MAIN(TestStoredCompletions)
#include "tst_storedcompletions.moc"
