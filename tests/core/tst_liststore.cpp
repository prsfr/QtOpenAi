// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/ChatCompletionList.h>

#include <QtCore/QJsonDocument>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the generic ListPage<T> and the ChatCompletion list typedefs.
class TestListStore : public QObject
{
    Q_OBJECT
private slots:
    void completionListRoundTrip();
    void messageListRoundTrip();
    void parsesListPage();
};

void TestListStore::completionListRoundTrip()
{
    ChatCompletionResponse a;
    a.setId(QStringLiteral("cmpl_1"));
    a.setModel(QStringLiteral("gpt-4o"));
    ChatCompletionResponse b;
    b.setId(QStringLiteral("cmpl_2"));
    b.setModel(QStringLiteral("gpt-4o"));

    ChatCompletionList list;
    list.data = {a, b};
    list.firstId = QStringLiteral("cmpl_1");
    list.lastId = QStringLiteral("cmpl_2");
    list.hasMore = true;

    const ChatCompletionList parsed = ChatCompletionList::fromJson(list.toJson());
    QCOMPARE(parsed, list);
    QCOMPARE(parsed.size(), 2);
    QVERIFY(parsed.hasMore);
    QCOMPARE(parsed.data.at(1).id(), QStringLiteral("cmpl_2"));
}

void TestListStore::messageListRoundTrip()
{
    ChatCompletionMessageList list;
    list.data = {Message::user(QStringLiteral("hi")), Message::assistant(QStringLiteral("hello"))};
    list.firstId = QStringLiteral("msg_1");
    list.lastId = QStringLiteral("msg_2");

    const ChatCompletionMessageList parsed = ChatCompletionMessageList::fromJson(list.toJson());
    QCOMPARE(parsed, list);
    QCOMPARE(parsed.data.size(), 2);
    QVERIFY(!parsed.hasMore);
}

void TestListStore::parsesListPage()
{
    const QByteArray body = R"({
        "object": "list",
        "data": [
            {"id": "cmpl_1", "object": "chat.completion", "created": 1, "model": "gpt-4o",
             "choices": [], "usage": {"prompt_tokens": 1, "completion_tokens": 1,
             "total_tokens": 2}}],
        "first_id": "cmpl_1", "last_id": "cmpl_1", "has_more": false
    })";
    const ChatCompletionList list
            = ChatCompletionList::fromJson(QJsonDocument::fromJson(body).object());
    QCOMPARE(list.size(), 1);
    QCOMPARE(list.data.first().id(), QStringLiteral("cmpl_1"));
    QCOMPARE(list.firstId, QStringLiteral("cmpl_1"));
    QVERIFY(!list.hasMore);
}

QTEST_APPLESS_MAIN(TestListStore)
#include "tst_liststore.moc"
