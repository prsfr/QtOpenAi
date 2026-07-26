// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

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

    const auto reply = awaited(client.createConversation(
            QJsonObject {{QStringLiteral("topic"), QStringLiteral("weather")}}));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/conversations "));
    QCOMPARE(reply->conversation().id(), QStringLiteral("conv_1"));
    QVERIFY(server.requestBody().contains("\"metadata\""));
}

void TestConversationsClient::listItemsUsesGetAndParsesPage()
{
    StubServer server(R"({"object":"list","data":[
        {"type":"message","id":"msg_1","role":"user",
         "content":[{"type":"input_text","text":"Hello"}]}],
        "first_id":"msg_1","last_id":"msg_1","has_more":false})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.listConversationItems(QStringLiteral("conv_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/conversations/conv_1/items "));
    QCOMPARE(reply->items().size(), 1);
    QCOMPARE(reply->firstItem().text(), QStringLiteral("Hello"));
}

void TestConversationsClient::createItemsPostsBody()
{
    StubServer server(R"({"object":"list","data":[
        {"type":"message","id":"msg_1","role":"user",
         "content":[{"type":"input_text","text":"Hi"}]}],
        "first_id":"msg_1","last_id":"msg_1","has_more":false})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.createConversationItems(
            QStringLiteral("conv_1"),
            {ResponseOutputItem::message(QStringLiteral("Hi"), QStringLiteral("user"))}));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/conversations/conv_1/items "));
    QVERIFY(server.requestBody().contains("\"items\""));
}

void TestConversationsClient::getItemWrapsSingleItem()
{
    StubServer server(R"({"type":"message","id":"msg_1","role":"assistant",
        "content":[{"type":"output_text","text":"Answer","annotations":[]}]})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(
            client.getConversationItem(QStringLiteral("conv_1"), QStringLiteral("msg_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/conversations/conv_1/items/msg_1 "));
    QCOMPARE(reply->items().size(), 1);
    QCOMPARE(reply->firstItem().text(), QStringLiteral("Answer"));
}

void TestConversationsClient::deleteConversationUsesDelete()
{
    StubServer server(R"({"id":"conv_1","object":"conversation.deleted","deleted":true})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.deleteConversation(QStringLiteral("conv_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("DELETE /v1/conversations/conv_1 "));
    QCOMPARE(reply->conversation().object(), QStringLiteral("conversation.deleted"));
}

QTEST_MAIN(TestConversationsClient)
#include "tst_conversations.moc"
