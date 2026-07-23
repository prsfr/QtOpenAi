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

class TestConversationsClient : public QObject
{
    Q_OBJECT
private slots:
    void createConversationPostsAndParses();
    void listItemsUsesGetAndParsesPage();
    void createItemsPostsBody();
    void getItemWrapsSingleItem();
    void deleteConversationUsesDelete();
};

void TestConversationsClient::createConversationPostsAndParses()
{
    StubServer server(R"({"id":"conv_1","object":"conversation","created_at":1,
        "metadata":{"topic":"weather"}})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    ConversationReply *reply = client.createConversation(
            QJsonObject {{QStringLiteral("topic"), QStringLiteral("weather")}});
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/conversations "));
    QCOMPARE(reply->conversation().id(), QStringLiteral("conv_1"));
    QVERIFY(server.requestBody().contains("\"metadata\""));
    delete reply;
}

void TestConversationsClient::listItemsUsesGetAndParsesPage()
{
    StubServer server(R"({"object":"list","data":[
        {"type":"message","id":"msg_1","role":"user",
         "content":[{"type":"input_text","text":"Hello"}]}],
        "first_id":"msg_1","last_id":"msg_1","has_more":false})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    ConversationItemsReply *reply = client.listConversationItems(QStringLiteral("conv_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/conversations/conv_1/items "));
    QCOMPARE(reply->items().items().size(), 1);
    QCOMPARE(reply->firstItem().text(), QStringLiteral("Hello"));
    delete reply;
}

void TestConversationsClient::createItemsPostsBody()
{
    StubServer server(R"({"object":"list","data":[
        {"type":"message","id":"msg_1","role":"user",
         "content":[{"type":"input_text","text":"Hi"}]}],
        "first_id":"msg_1","last_id":"msg_1","has_more":false})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    ConversationItemsReply *reply = client.createConversationItems(
            QStringLiteral("conv_1"),
            {ResponseOutputItem::message(QStringLiteral("Hi"), QStringLiteral("user"))});
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/conversations/conv_1/items "));
    QVERIFY(server.requestBody().contains("\"items\""));
    delete reply;
}

void TestConversationsClient::getItemWrapsSingleItem()
{
    StubServer server(R"({"type":"message","id":"msg_1","role":"assistant",
        "content":[{"type":"output_text","text":"Answer","annotations":[]}]})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    ConversationItemsReply *reply
            = client.getConversationItem(QStringLiteral("conv_1"), QStringLiteral("msg_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/conversations/conv_1/items/msg_1 "));
    QCOMPARE(reply->items().items().size(), 1);
    QCOMPARE(reply->firstItem().text(), QStringLiteral("Answer"));
    delete reply;
}

void TestConversationsClient::deleteConversationUsesDelete()
{
    StubServer server(R"({"id":"conv_1","object":"conversation.deleted","deleted":true})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    ConversationReply *reply = client.deleteConversation(QStringLiteral("conv_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("DELETE /v1/conversations/conv_1 "));
    QCOMPARE(reply->conversation().object(), QStringLiteral("conversation.deleted"));
    delete reply;
}

QTEST_MAIN(TestConversationsClient)
#include "tst_conversations.moc"
