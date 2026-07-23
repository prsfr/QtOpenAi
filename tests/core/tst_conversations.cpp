// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/Conversation.h>
#include <QtOpenAi/Core/ConversationItemList.h>

#include <QtCore/QJsonDocument>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// JSON round-trip coverage for the Conversations-API value types.
class TestConversations : public QObject
{
    Q_OBJECT
private slots:
    void conversationRoundTrip();
    void itemListRoundTrip();
    void parsesItemListPage();
    void parsesInputTextMessage();
    void valueSemantics();
};

void TestConversations::conversationRoundTrip()
{
    Conversation conversation;
    conversation.setId(QStringLiteral("conv_1"));
    conversation.setCreatedAt(1700000000);
    conversation.setMetadata(QJsonObject {{QStringLiteral("topic"), QStringLiteral("weather")}});

    const Conversation parsed = Conversation::fromJson(conversation.toJson());
    QCOMPARE(parsed, conversation);
    QCOMPARE(parsed.object(), QStringLiteral("conversation"));
}

void TestConversations::itemListRoundTrip()
{
    ConversationItemList list;
    list.setItems({ResponseOutputItem::message(QStringLiteral("hi")),
                   ResponseOutputItem::functionCall(QStringLiteral("f"), QStringLiteral("{}"),
                                                    QStringLiteral("call_1"))});
    list.setFirstId(QStringLiteral("msg_1"));
    list.setLastId(QStringLiteral("fc_1"));
    list.setHasMore(true);

    const ConversationItemList parsed = ConversationItemList::fromJson(list.toJson());
    QCOMPARE(parsed, list);
    QCOMPARE(parsed.items().size(), 2);
    QVERIFY(parsed.hasMore());
}

void TestConversations::parsesItemListPage()
{
    const QByteArray body = R"({
        "object": "list",
        "data": [
            {"type": "message", "id": "msg_1", "role": "user",
             "content": [{"type": "input_text", "text": "Hello"}]},
            {"type": "message", "id": "msg_2", "role": "assistant",
             "content": [{"type": "output_text", "text": "Hi there", "annotations": []}]}
        ],
        "first_id": "msg_1", "last_id": "msg_2", "has_more": false
    })";
    const ConversationItemList list
            = ConversationItemList::fromJson(QJsonDocument::fromJson(body).object());

    QCOMPARE(list.items().size(), 2);
    QCOMPARE(list.items().at(0).text(), QStringLiteral("Hello"));
    QCOMPARE(list.items().at(1).text(), QStringLiteral("Hi there"));
    QCOMPARE(list.firstId(), QStringLiteral("msg_1"));
    QVERIFY(!list.hasMore());
}

void TestConversations::parsesInputTextMessage()
{
    // A conversation user message uses input_text content parts.
    const QByteArray body = R"({"type": "message", "id": "msg_x", "role": "user",
        "content": [{"type": "input_text", "text": "part one "},
                    {"type": "input_text", "text": "part two"}]})";
    const ResponseOutputItem item
            = ResponseOutputItem::fromJson(QJsonDocument::fromJson(body).object());
    QVERIFY(item.isMessage());
    QCOMPARE(item.text(), QStringLiteral("part one part two"));
}

void TestConversations::valueSemantics()
{
    Conversation a;
    a.setId(QStringLiteral("conv_1"));
    Conversation b = a;
    QCOMPARE(a, b);
    b.setId(QStringLiteral("conv_2"));
    QVERIFY(a != b);
    QCOMPARE(a.id(), QStringLiteral("conv_1"));
}

QTEST_APPLESS_MAIN(TestConversations)
#include "tst_conversations.moc"
