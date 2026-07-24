// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/StubServer.h"

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
