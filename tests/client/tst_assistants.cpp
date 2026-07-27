// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

// Offline stub-server coverage for the Assistants endpoints (#23): the CRUD
// surface plus the beta header the whole family has to carry.
class TestAssistantsClient : public QObject
{
    Q_OBJECT
private slots:
    void createPostsJsonBodyWithBetaHeader();
    void listSendsPaginationQueryWithBetaHeader();
    void getParsesAssistant();
    void updatePostsOnlyChangedFields();
    void deleteIssuesDeleteVerbWithBetaHeader();
    void defaultHeaderOverridesTheBetaVersion();
};

void TestAssistantsClient::createPostsJsonBodyWithBetaHeader()
{
    StubServer server(QByteArray(R"({"id":"asst_1","object":"assistant","name":"Weather bot",)"
                                 R"("model":"gpt-4o-mini"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateAssistantRequest request(QStringLiteral("gpt-4o-mini"));
    request.setName(QStringLiteral("Weather bot"));
    request.addTool(
            Tool::function(QStringLiteral("get_weather"), QStringLiteral("Weather"),
                           QJsonObject {{QStringLiteral("type"), QStringLiteral("object")}}));

    const auto reply = awaited(client.createAssistant(request));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/assistants "));
    // The API rejects an Assistants call that does not name the beta it speaks.
    QVERIFY(server.requestHeaders().toLower().contains("openai-beta: assistants=v2"));
    QVERIFY(server.requestBody().contains("\"model\":\"gpt-4o-mini\""));
    QVERIFY(server.requestBody().contains("\"name\":\"get_weather\""));
    QCOMPARE(reply->assistant().id(), QStringLiteral("asst_1"));
}

void TestAssistantsClient::listSendsPaginationQueryWithBetaHeader()
{
    StubServer server(QByteArray(R"({"object":"list","data":[{"id":"asst_1"},{"id":"asst_2"}],)"
                                 R"("first_id":"asst_1","last_id":"asst_2","has_more":true})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.limit = 2;
    params.order = QStringLiteral("desc");
    const auto reply = awaited(client.listAssistants(params));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/assistants?"));
    QVERIFY(server.requestLine().contains("limit=2"));
    QVERIFY(server.requestLine().contains("order=desc"));
    QVERIFY(server.requestHeaders().toLower().contains("openai-beta: assistants=v2"));
    QCOMPARE(reply->list().size(), 2);
    QCOMPARE(reply->list().lastId, QStringLiteral("asst_2"));
    QVERIFY(reply->list().hasMore);
}

void TestAssistantsClient::getParsesAssistant()
{
    StubServer server(QByteArray(R"({"id":"asst_1","created_at":1716028800,)"
                                 R"("instructions":"Be concise.","temperature":0.4,)"
                                 R"("tools":[{"type":"file_search"}]})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.getAssistant(QStringLiteral("asst_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/assistants/asst_1 "));
    QCOMPARE(reply->assistant().createdAt(), Q_INT64_C(1716028800));
    QCOMPARE(reply->assistant().instructions(), QStringLiteral("Be concise."));
    QCOMPARE(reply->assistant().temperature().value(), 0.4);
    QCOMPARE(reply->assistant().tools().size(), 1);
}

void TestAssistantsClient::updatePostsOnlyChangedFields()
{
    StubServer server(QByteArray(R"({"id":"asst_1","name":"Renamed"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateAssistantRequest request;
    request.setName(QStringLiteral("Renamed"));

    const auto reply = awaited(client.updateAssistant(QStringLiteral("asst_1"), request));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/assistants/asst_1 "));
    // An update carries the changes and nothing else, so it cannot reset a
    // field the caller never touched.
    QCOMPARE(server.requestBody(), QByteArray(R"({"name":"Renamed"})"));
    QCOMPARE(reply->assistant().name(), QStringLiteral("Renamed"));
}

void TestAssistantsClient::deleteIssuesDeleteVerbWithBetaHeader()
{
    StubServer server(QByteArray(R"({"id":"asst_1","object":"assistant.deleted","deleted":true})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.deleteAssistant(QStringLiteral("asst_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("DELETE /v1/assistants/asst_1 "));
    QVERIFY(server.requestHeaders().toLower().contains("openai-beta: assistants=v2"));
    QCOMPARE(reply->assistant().object(), QStringLiteral("assistant.deleted"));
}

void TestAssistantsClient::defaultHeaderOverridesTheBetaVersion()
{
    // The library picks assistants=v2, but a caller talking to a provider that
    // expects a different value can still say so -- default headers are applied
    // last and win.
    StubServer server(QByteArray(R"({"id":"asst_1"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));
    client.setDefaultHeader("OpenAI-Beta", "assistants=v1");

    const auto reply = awaited(client.getAssistant(QStringLiteral("asst_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    const QByteArray headers = server.requestHeaders().toLower();
    QVERIFY(headers.contains("openai-beta: assistants=v1"));
    QVERIFY(!headers.contains("assistants=v2"));
}

QTEST_MAIN(TestAssistantsClient)
#include "tst_assistants.moc"
