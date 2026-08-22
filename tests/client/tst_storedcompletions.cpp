// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtCore/QJsonDocument>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/AwaitedReply.h"
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
    void updateSendsMetadataAndReadsItBack();
    void untaggedCompletionDecodesToEmptyMetadata();
    void deleteUsesDelete();
    void listMessagesUsesGet();
};

void TestStoredCompletions::getUsesGetAndParses()
{
    StubServer server(completionBody());
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.getChatCompletion(QStringLiteral("cmpl_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/chat/completions/cmpl_1 "));
    QCOMPARE(reply->response().id(), QStringLiteral("cmpl_1"));
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
    const auto reply = awaited(client.listChatCompletions(params));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/chat/completions?"));
    QVERIFY(server.requestLine().contains("limit=5"));
    QVERIFY(server.requestLine().contains("after=cmpl_0"));
    QCOMPARE(reply->list().size(), 1);
    QVERIFY(reply->list().hasMore);
}

void TestStoredCompletions::updateSendsMetadataAndReadsItBack()
{
    StubServer server(R"({"id":"cmpl_1","object":"chat.completion","created":1,"model":"gpt-4o",
        "choices":[],"usage":{"prompt_tokens":1,"completion_tokens":1,"total_tokens":2},
        "metadata":{"topic":"billing","ticket":"4711"}})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    QJsonObject metadata;
    metadata.insert(QStringLiteral("topic"), QStringLiteral("billing"));
    metadata.insert(QStringLiteral("ticket"), QStringLiteral("4711"));

    const auto reply = awaited(client.updateChatCompletion(QStringLiteral("cmpl_1"), metadata));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/chat/completions/cmpl_1 "));
    // The body wraps the tags: {"metadata": {...}}, not the tags at the top level.
    const QJsonObject sent = QJsonDocument::fromJson(server.requestBody()).object();
    QCOMPARE(sent.value(QStringLiteral("metadata")).toObject(), metadata);

    // Reading it back is the half that used to be missing: the tags could be
    // written and never retrieved through the typed value.
    QCOMPARE(reply->response().metadata(), metadata);
}

void TestStoredCompletions::untaggedCompletionDecodesToEmptyMetadata()
{
    // The API sends `"metadata": null` for a stored completion nobody tagged,
    // and omits the field entirely on the reply to createChatCompletion().
    // Both have to arrive as an empty object rather than as a decode failure.
    StubServer server(QList<StubServer::Response> {
            {R"({"id":"cmpl_1","object":"chat.completion","created":1,"model":"gpt-4o",
                 "choices":[],"usage":{},"metadata":null})"},
            {completionBody()},
    });
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto nulled = awaited(client.getChatCompletion(QStringLiteral("cmpl_1")));
    QVERIFY(nulled);
    QVERIFY(nulled->isSuccess());
    QVERIFY(nulled->response().metadata().isEmpty());

    const auto absent = awaited(client.getChatCompletion(QStringLiteral("cmpl_1")));
    QVERIFY(absent);
    QVERIFY(absent->isSuccess());
    QVERIFY(absent->response().metadata().isEmpty());
    // ...and an empty one does not come back out on the way to JSON.
    QVERIFY(!absent->response().toJson().contains(QStringLiteral("metadata")));
}

void TestStoredCompletions::deleteUsesDelete()
{
    StubServer server(R"({"id":"cmpl_1","object":"chat.completion.deleted","deleted":true})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.deleteChatCompletion(QStringLiteral("cmpl_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("DELETE /v1/chat/completions/cmpl_1 "));
    QCOMPARE(reply->response().object(), QStringLiteral("chat.completion.deleted"));
}

void TestStoredCompletions::listMessagesUsesGet()
{
    StubServer server(R"({"object":"list","data":[
        {"role":"user","content":"hi"}],
        "first_id":"msg_1","last_id":"msg_1","has_more":false})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.listChatCompletionMessages(QStringLiteral("cmpl_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/chat/completions/cmpl_1/messages "));
    QCOMPARE(reply->list().size(), 1);
}

QTEST_MAIN(TestStoredCompletions)
#include "tst_storedcompletions.moc"
