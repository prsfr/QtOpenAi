// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/ChatKitSession.h>
#include <QtOpenAi/Core/ChatKitThread.h>
#include <QtOpenAi/Core/ChatKitThreadItem.h>
#include <QtOpenAi/Core/CreateChatKitSessionRequest.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the ChatKit types (#26): the session that mints an ephemeral
// client secret, the threads its UI produces, and the heterogeneous items
// inside them.
class TestChatKit : public QObject
{
    Q_OBJECT
private slots:
    void parsesSession();
    void sessionRoundTrip();
    void unknownSessionStatusDecodesToActive();
    void requestSerialisesBody();
    void requestOmitsUnsetFields();
    void parsesThread();
    void threadStatusRoundTripsAsAnObject();
    void threadStatusReasonSurvives();
    void parsesUserMessageItem();
    void parsesAssistantMessageItem();
    void unmodelledItemFieldsSurviveRoundTrip();
    void parsesThreadItemList();
};

void TestChatKit::parsesSession()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("cksess_123")},
            {QStringLiteral("object"), QStringLiteral("chatkit.session")},
            {QStringLiteral("expires_at"), 1712349876},
            {QStringLiteral("client_secret"), QStringLiteral("ek_token_123")},
            {QStringLiteral("user"), QStringLiteral("user_789")},
            {QStringLiteral("max_requests_per_1_minute"), 60},
            {QStringLiteral("status"), QStringLiteral("cancelled")},
            {QStringLiteral("workflow"),
             QJsonObject {{QStringLiteral("id"), QStringLiteral("workflow_alpha")},
                          {QStringLiteral("version"), QStringLiteral("2024-10-01")}}},
            {QStringLiteral("chatkit_configuration"),
             QJsonObject {{QStringLiteral("file_upload"),
                           QJsonObject {{QStringLiteral("enabled"), true}}}}},
    };

    const ChatKitSession session = ChatKitSession::fromJson(json);
    QCOMPARE(session.id(), QStringLiteral("cksess_123"));
    QCOMPARE(session.object(), QStringLiteral("chatkit.session"));
    QCOMPARE(session.expiresAt(), Q_INT64_C(1712349876));
    // The ephemeral secret is the whole point of the endpoint: it is what the
    // browser gets, in place of the API key.
    QCOMPARE(session.clientSecret(), QStringLiteral("ek_token_123"));
    QCOMPARE(session.user(), QStringLiteral("user_789"));
    QCOMPARE(session.maxRequestsPerMinute(), 60);
    QCOMPARE(session.status(), ChatKitSessionStatus::Cancelled);
    QCOMPARE(session.workflow().value(QStringLiteral("id")).toString(),
             QStringLiteral("workflow_alpha"));
    QVERIFY(session.configuration().contains(QStringLiteral("file_upload")));
}

void TestChatKit::sessionRoundTrip()
{
    ChatKitSession session;
    session.setId(QStringLiteral("cksess_123"));
    session.setObject(QStringLiteral("chatkit.session"));
    session.setExpiresAt(1712349876);
    session.setClientSecret(QStringLiteral("ek_token_123"));
    session.setUser(QStringLiteral("user_789"));
    session.setMaxRequestsPerMinute(60);
    session.setStatus(ChatKitSessionStatus::Active);
    session.setWorkflow(QJsonObject {{QStringLiteral("id"), QStringLiteral("workflow_alpha")}});
    session.setConfiguration(QJsonObject {{QStringLiteral("history"), QJsonObject {}}});

    QCOMPARE(ChatKitSession::fromJson(session.toJson()), session);
}

void TestChatKit::unknownSessionStatusDecodesToActive()
{
    const QJsonObject json {
            {QStringLiteral("status"), QStringLiteral("hibernating")},
    };

    // A status from a newer server must not read as "this session is over".
    QCOMPARE(ChatKitSession::fromJson(json).status(), ChatKitSessionStatus::Active);
}

void TestChatKit::requestSerialisesBody()
{
    CreateChatKitSessionRequest request(QStringLiteral("workflow_alpha"),
                                        QStringLiteral("user_789"));
    request.setWorkflowVersion(QStringLiteral("2024-10-01"));
    request.setWorkflowStateVariables(
            QJsonObject {{QStringLiteral("tier"), QStringLiteral("pro")}});
    request.setWorkflowTracingEnabled(false);
    request.setExpiresAfter(300);
    request.setMaxRequestsPerMinute(30);
    request.setConfiguration(QJsonObject {
            {QStringLiteral("history"), QJsonObject {{QStringLiteral("enabled"), false}}}});

    const QJsonObject json = request.toJson();
    const QJsonObject workflow = json.value(QStringLiteral("workflow")).toObject();
    QCOMPARE(workflow.value(QStringLiteral("id")).toString(), QStringLiteral("workflow_alpha"));
    QCOMPARE(workflow.value(QStringLiteral("version")).toString(), QStringLiteral("2024-10-01"));
    QCOMPARE(workflow.value(QStringLiteral("state_variables"))
                     .toObject()
                     .value(QStringLiteral("tier"))
                     .toString(),
             QStringLiteral("pro"));
    QCOMPARE(workflow.value(QStringLiteral("tracing"))
                     .toObject()
                     .value(QStringLiteral("enabled"))
                     .toBool(),
             false);
    QCOMPARE(json.value(QStringLiteral("user")).toString(), QStringLiteral("user_789"));
    // The anchor is fixed to created_at by the API, so it is sent along with the
    // seconds rather than left to the caller.
    const QJsonObject expires = json.value(QStringLiteral("expires_after")).toObject();
    QCOMPARE(expires.value(QStringLiteral("anchor")).toString(), QStringLiteral("created_at"));
    QCOMPARE(expires.value(QStringLiteral("seconds")).toInt(), 300);
    QCOMPARE(json.value(QStringLiteral("rate_limits"))
                     .toObject()
                     .value(QStringLiteral("max_requests_per_1_minute"))
                     .toInt(),
             30);
    QVERIFY(json.contains(QStringLiteral("chatkit_configuration")));
}

void TestChatKit::requestOmitsUnsetFields()
{
    const CreateChatKitSessionRequest request(QStringLiteral("workflow_alpha"),
                                              QStringLiteral("user_789"));

    const QJsonObject json = request.toJson();
    // Only the two required fields; every override is the server's default
    // until the caller says otherwise.
    QCOMPARE(json.keys(), QStringList({QStringLiteral("user"), QStringLiteral("workflow")}));
    QCOMPARE(json.value(QStringLiteral("workflow")).toObject().keys(),
             QStringList({QStringLiteral("id")}));
}

void TestChatKit::parsesThread()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("cthr_def456")},
            {QStringLiteral("object"), QStringLiteral("chatkit.thread")},
            {QStringLiteral("created_at"), 1712345600},
            {QStringLiteral("title"), QStringLiteral("Demo feedback")},
            {QStringLiteral("status"),
             QJsonObject {{QStringLiteral("type"), QStringLiteral("active")}}},
            {QStringLiteral("user"), QStringLiteral("user_456")},
    };

    const ChatKitThread thread = ChatKitThread::fromJson(json);
    QCOMPARE(thread.id(), QStringLiteral("cthr_def456"));
    QCOMPARE(thread.createdAt(), Q_INT64_C(1712345600));
    QCOMPARE(thread.title(), QStringLiteral("Demo feedback"));
    QCOMPARE(thread.status(), ChatKitThreadStatus::Active);
    QVERIFY(thread.statusReason().isEmpty());
    QCOMPARE(thread.user(), QStringLiteral("user_456"));
}

void TestChatKit::threadStatusRoundTripsAsAnObject()
{
    ChatKitThread thread;
    thread.setId(QStringLiteral("cthr_def456"));
    thread.setObject(QStringLiteral("chatkit.thread"));
    thread.setCreatedAt(1712345600);
    thread.setTitle(QStringLiteral("Demo feedback"));
    thread.setStatus(ChatKitThreadStatus::Closed);
    thread.setStatusReason(QStringLiteral("workflow finished"));
    thread.setUser(QStringLiteral("user_456"));

    // Unlike every other status in the library this one is a tagged object on
    // the wire, not a bare string.
    const QJsonObject status = thread.toJson().value(QStringLiteral("status")).toObject();
    QCOMPARE(status.value(QStringLiteral("type")).toString(), QStringLiteral("closed"));
    QCOMPARE(status.value(QStringLiteral("reason")).toString(),
             QStringLiteral("workflow finished"));
    QCOMPARE(ChatKitThread::fromJson(thread.toJson()), thread);
}

void TestChatKit::threadStatusReasonSurvives()
{
    const QJsonObject json {
            {QStringLiteral("status"),
             QJsonObject {{QStringLiteral("type"), QStringLiteral("locked")},
                          {QStringLiteral("reason"), QStringLiteral("moderation hold")}}},
    };

    const ChatKitThread thread = ChatKitThread::fromJson(json);
    QCOMPARE(thread.status(), ChatKitThreadStatus::Locked);
    QCOMPARE(thread.statusReason(), QStringLiteral("moderation hold"));
}

void TestChatKit::parsesUserMessageItem()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("cthritm_1")},
            {QStringLiteral("object"), QStringLiteral("chatkit.thread_item")},
            {QStringLiteral("created_at"), 1712345601},
            {QStringLiteral("thread_id"), QStringLiteral("cthr_def456")},
            {QStringLiteral("type"), QStringLiteral("chatkit.user_message")},
            {QStringLiteral("content"),
             QJsonArray {QJsonObject {{QStringLiteral("type"), QStringLiteral("input_text")},
                                      {QStringLiteral("text"), QStringLiteral("Hello ")}},
                         QJsonObject {{QStringLiteral("type"), QStringLiteral("quoted_text")},
                                      {QStringLiteral("text"), QStringLiteral("there")}}}},
    };

    const ChatKitThreadItem item = ChatKitThreadItem::fromJson(json);
    QCOMPARE(item.id(), QStringLiteral("cthritm_1"));
    QCOMPARE(item.threadId(), QStringLiteral("cthr_def456"));
    QVERIFY(item.isUserMessage());
    QVERIFY(!item.isAssistantMessage());
    QCOMPARE(item.content().size(), 2);
    // Both user content blocks carry their text flat, so both read back.
    QCOMPARE(item.text(), QStringLiteral("Hello there"));
}

void TestChatKit::parsesAssistantMessageItem()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("cthritm_2")},
            {QStringLiteral("type"), QStringLiteral("chatkit.assistant_message")},
            {QStringLiteral("content"),
             QJsonArray {QJsonObject {{QStringLiteral("type"), QStringLiteral("output_text")},
                                      {QStringLiteral("text"), QStringLiteral("Hi!")}}}},
    };

    const ChatKitThreadItem item = ChatKitThreadItem::fromJson(json);
    QVERIFY(item.isAssistantMessage());
    QCOMPARE(item.text(), QStringLiteral("Hi!"));
}

void TestChatKit::unmodelledItemFieldsSurviveRoundTrip()
{
    // A thread item is a six-way union; only the envelope and the message
    // content are typed, so everything else has to survive verbatim rather than
    // be dropped.
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("cthritm_3")},
            {QStringLiteral("object"), QStringLiteral("chatkit.thread_item")},
            {QStringLiteral("created_at"), 1712345602},
            {QStringLiteral("thread_id"), QStringLiteral("cthr_def456")},
            {QStringLiteral("type"), QStringLiteral("chatkit.client_tool_call")},
            {QStringLiteral("call_id"), QStringLiteral("call_1")},
            {QStringLiteral("name"), QStringLiteral("lookup")},
            {QStringLiteral("arguments"), QStringLiteral(R"({"q":"qt"})")},
            {QStringLiteral("status"), QStringLiteral("completed")},
    };

    const ChatKitThreadItem item = ChatKitThreadItem::fromJson(json);
    QVERIFY(!item.isUserMessage());
    QCOMPARE(item.type(), QStringLiteral("chatkit.client_tool_call"));
    QCOMPARE(item.raw().value(QStringLiteral("name")).toString(), QStringLiteral("lookup"));
    QVERIFY(item.text().isEmpty());
    QCOMPARE(item.toJson(), json);
}

void TestChatKit::parsesThreadItemList()
{
    const QJsonObject json {
            {QStringLiteral("object"), QStringLiteral("list")},
            {QStringLiteral("data"),
             QJsonArray {QJsonObject {{QStringLiteral("id"), QStringLiteral("cthritm_1")}},
                         QJsonObject {{QStringLiteral("id"), QStringLiteral("cthritm_2")}}}},
            {QStringLiteral("first_id"), QStringLiteral("cthritm_1")},
            {QStringLiteral("last_id"), QStringLiteral("cthritm_2")},
            {QStringLiteral("has_more"), true},
    };

    const ChatKitThreadItemList list = ChatKitThreadItemList::fromJson(json);
    QCOMPARE(list.size(), 2);
    QCOMPARE(list.data.at(1).id(), QStringLiteral("cthritm_2"));
    QVERIFY(list.hasMore);
}

QTEST_MAIN(TestChatKit)
#include "tst_chatkit.moc"
