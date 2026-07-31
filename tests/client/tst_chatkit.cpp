// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

// Offline stub-server coverage for the ChatKit endpoints (#26): minting and
// cancelling a session, and the read-only thread surface below it — plus the
// beta header the whole family has to carry.
class TestChatKitClient : public QObject
{
    Q_OBJECT
private slots:
    void createSessionPostsJsonBodyWithBetaHeader();
    void cancelSessionPostsToCancelPath();
    void listThreadsSendsPaginationQuery();
    void listThreadsFiltersByUser();
    void getThreadParsesTaggedStatus();
    void deleteThreadIssuesDeleteVerbWithBetaHeader();
    void listThreadItemsParsesUnionItems();
    void defaultHeaderOverridesTheBetaVersion();
};

void TestChatKitClient::createSessionPostsJsonBodyWithBetaHeader()
{
    StubServer server(QByteArray(R"({"id":"cksess_123","object":"chatkit.session",)"
                                 R"("client_secret":"ek_token_123","expires_at":1712349876,)"
                                 R"("status":"active","max_requests_per_1_minute":30})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateChatKitSessionRequest request(QStringLiteral("workflow_alpha"),
                                        QStringLiteral("user_789"));
    request.setExpiresAfter(300);

    const auto reply = awaited(client.createChatKitSession(request));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/chatkit/sessions "));
    // The API rejects a ChatKit call that does not name the beta it speaks.
    QVERIFY(server.requestHeaders().toLower().contains("openai-beta: chatkit_beta=v1"));
    QVERIFY(server.requestBody().contains(R"("id":"workflow_alpha")"));
    QVERIFY(server.requestBody().contains(R"("seconds":300)"));
    QCOMPARE(reply->session().clientSecret(), QStringLiteral("ek_token_123"));
    QCOMPARE(reply->session().status(), ChatKitSessionStatus::Active);
}

void TestChatKitClient::cancelSessionPostsToCancelPath()
{
    StubServer server(QByteArray(R"({"id":"cksess_123","status":"cancelled"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.cancelChatKitSession(QStringLiteral("cksess_123")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/chatkit/sessions/cksess_123/cancel "));
    QVERIFY(server.requestHeaders().toLower().contains("openai-beta: chatkit_beta=v1"));
    QCOMPARE(reply->session().status(), ChatKitSessionStatus::Cancelled);
}

void TestChatKitClient::listThreadsSendsPaginationQuery()
{
    StubServer server(QByteArray(R"({"object":"list","data":[{"id":"cthr_1"},{"id":"cthr_2"}],)"
                                 R"("first_id":"cthr_1","last_id":"cthr_2","has_more":true})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.limit = 2;
    params.order = QStringLiteral("desc");
    const auto reply = awaited(client.listChatKitThreads(params));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/chatkit/threads?"));
    QVERIFY(server.requestLine().contains("limit=2"));
    QVERIFY(server.requestLine().contains("order=desc"));
    QCOMPARE(reply->list().size(), 2);
    QVERIFY(reply->list().hasMore);
}

void TestChatKitClient::listThreadsFiltersByUser()
{
    // Threads belong to end users, so the listing takes a `user` filter on top
    // of the shared pagination parameters.
    StubServer server(QByteArray(R"({"object":"list","data":[],"has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.listChatKitThreads({}, QStringLiteral("user_456")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().contains("user=user_456"));
}

void TestChatKitClient::getThreadParsesTaggedStatus()
{
    StubServer server(QByteArray(R"({"id":"cthr_1","object":"chatkit.thread",)"
                                 R"("created_at":1712345600,"title":"Demo feedback",)"
                                 R"("status":{"type":"locked","reason":"moderation hold"},)"
                                 R"("user":"user_456"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.getChatKitThread(QStringLiteral("cthr_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/chatkit/threads/cthr_1 "));
    QCOMPARE(reply->thread().title(), QStringLiteral("Demo feedback"));
    QCOMPARE(reply->thread().status(), ChatKitThreadStatus::Locked);
    QCOMPARE(reply->thread().statusReason(), QStringLiteral("moderation hold"));
}

void TestChatKitClient::deleteThreadIssuesDeleteVerbWithBetaHeader()
{
    StubServer server(QByteArray(R"({"id":"cthr_1","object":"chatkit.thread.deleted",)"
                                 R"("deleted":true})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.deleteChatKitThread(QStringLiteral("cthr_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("DELETE /v1/chatkit/threads/cthr_1 "));
    QVERIFY(server.requestHeaders().toLower().contains("openai-beta: chatkit_beta=v1"));
    QCOMPARE(reply->thread().object(), QStringLiteral("chatkit.thread.deleted"));
}

void TestChatKitClient::listThreadItemsParsesUnionItems()
{
    StubServer server(
            QByteArray(R"({"object":"list","data":[)"
                       R"({"id":"i_1","type":"chatkit.user_message","thread_id":"cthr_1",)"
                       R"("content":[{"type":"input_text","text":"Hi"}]},)"
                       R"({"id":"i_2","type":"chatkit.widget","widget":"{\"kind\":\"card\"}"}],)"
                       R"("has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.after = QStringLiteral("i_0");
    const auto reply = awaited(client.listChatKitThreadItems(QStringLiteral("cthr_1"), params));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/chatkit/threads/cthr_1/items?"));
    QVERIFY(server.requestLine().contains("after=i_0"));
    QCOMPARE(reply->list().size(), 2);
    QVERIFY(reply->list().data.first().isUserMessage());
    QCOMPARE(reply->list().data.first().text(), QStringLiteral("Hi"));
    // A variant this library does not model still arrives whole.
    QCOMPARE(reply->list().data.at(1).raw().value(QStringLiteral("widget")).toString(),
             QStringLiteral(R"({"kind":"card"})"));
}

void TestChatKitClient::defaultHeaderOverridesTheBetaVersion()
{
    // The library picks chatkit_beta=v1; a caller talking to a provider that
    // expects another value can still say so -- default headers win.
    StubServer server(QByteArray(R"({"id":"cthr_1"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));
    client.setDefaultHeader("OpenAI-Beta", "chatkit_beta=v2");

    const auto reply = awaited(client.getChatKitThread(QStringLiteral("cthr_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    const QByteArray headers = server.requestHeaders().toLower();
    QVERIFY(headers.contains("openai-beta: chatkit_beta=v2"));
    QVERIFY(!headers.contains("chatkit_beta=v1"));
}

QTEST_MAIN(TestChatKitClient)
#include "tst_chatkit.moc"
