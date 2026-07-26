// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

class TestResponsesClient : public QObject
{
    Q_OBJECT
private:
    static QByteArray responseBody(const char *status = "completed")
    {
        // Single balanced raw string (a placeholder keeps moc's brace counter
        // happy) with the status substituted in.
        QByteArray body = R"({"id":"resp_1","object":"response","created_at":1,"model":"gpt-5",
            "status":"__STATUS__","output":[{"type":"message","id":"msg_1","status":"completed",
            "role":"assistant","content":[{"type":"output_text","text":"hi","annotations":[]}]}],
            "usage":{"input_tokens":1,"output_tokens":1,"total_tokens":2,
            "output_tokens_details":{"reasoning_tokens":0}}})";
        body.replace("__STATUS__", status);
        return body;
    }

private slots:
    void createResponsePostsAndParses();
    void getResponseUsesGet();
    void cancelResponsePostsToCancel();
    void deleteResponseUsesDelete();
    void httpErrorSurfacesMessage();
    void listsInputItemsWithPagination();
    void compactsAStoredResponse();
    void countsInputTokens();
};

void TestResponsesClient::createResponsePostsAndParses()
{
    StubServer server(200, responseBody());
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(
            client.createResponse(ResponseRequest(QStringLiteral("gpt-5"), QStringLiteral("hi"))));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/responses "));
    QCOMPARE(reply->response().id(), QStringLiteral("resp_1"));
    QCOMPARE(reply->response().outputText(), QStringLiteral("hi"));
}

void TestResponsesClient::getResponseUsesGet()
{
    StubServer server(200, responseBody());
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.getResponse(QStringLiteral("resp_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/responses/resp_1 "));
}

void TestResponsesClient::cancelResponsePostsToCancel()
{
    StubServer server(200, responseBody("cancelled"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.cancelResponse(QStringLiteral("resp_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/responses/resp_1/cancel "));
    QCOMPARE(reply->response().status(), QStringLiteral("cancelled"));
}

void TestResponsesClient::deleteResponseUsesDelete()
{
    StubServer server(200, R"({"id":"resp_1","object":"response.deleted","deleted":true})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.deleteResponse(QStringLiteral("resp_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("DELETE /v1/responses/resp_1 "));
    QCOMPARE(reply->response().object(), QStringLiteral("response.deleted"));
}

void TestResponsesClient::httpErrorSurfacesMessage()
{
    StubServer server(404, R"({"error":{"message":"No response found","type":"invalid_request"}})");
    Client client(server.baseUrl(), QStringLiteral("k"));
    client.setRetryPolicy(RetryPolicy::none());

    const auto reply = awaited(client.getResponse(QStringLiteral("missing")));
    QVERIFY(reply);

    QVERIFY(!reply->isSuccess());
    QCOMPARE(reply->error().httpStatus(), 404);
    QCOMPARE(reply->error().message(), QStringLiteral("No response found"));
}

void TestResponsesClient::listsInputItemsWithPagination()
{
    // The input items of a stored response come back in the same list shape as
    // conversation items, so the reply type is shared.
    StubServer server(QByteArray(R"({"object":"list","data":[)"
                                 R"({"id":"msg_1","type":"message","role":"user"},)"
                                 R"({"id":"msg_2","type":"message","role":"user"}],)"
                                 R"("first_id":"msg_1","last_id":"msg_2","has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.limit = 2;
    const auto reply = awaited(client.listResponseInputItems(QStringLiteral("resp_1"), params));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/responses/resp_1/input_items?"));
    QVERIFY(server.requestLine().contains("limit=2"));
    QCOMPARE(reply->items().size(), 2);
    QCOMPARE(reply->items().lastId, QStringLiteral("msg_2"));
    QVERIFY(!reply->items().hasMore);
}

void TestResponsesClient::compactsAStoredResponse()
{
    StubServer server(QByteArray(R"({"id":"resp_1","object":"response","status":"completed"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.compactResponse(
            QStringLiteral("resp_1"), QJsonObject {{QStringLiteral("keep_last"), 4}}));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/responses/compact "));
    QVERIFY(server.requestBody().contains("\"response_id\":\"resp_1\""));
    // Unmodelled fields are passed through rather than dropped.
    QVERIFY(server.requestBody().contains("\"keep_last\":4"));
    QCOMPARE(reply->response().id(), QStringLiteral("resp_1"));
}

void TestResponsesClient::countsInputTokens()
{
    StubServer server(QByteArray(R"({"object":"response.input_tokens","input_tokens":1234})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ResponseRequest request(QStringLiteral("gpt-4o-mini"), QStringLiteral("hello"));
    const auto reply = awaited(client.countResponseInputTokens(request));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/responses/input_tokens "));
    QVERIFY(server.requestBody().contains("\"model\":\"gpt-4o-mini\""));
    QCOMPARE(reply->inputTokens(), Q_INT64_C(1234));
    // The whole payload stays reachable for fields this library does not name.
    QCOMPARE(reply->object().value(QStringLiteral("object")).toString(),
             QStringLiteral("response.input_tokens"));
}

QTEST_MAIN(TestResponsesClient)
#include "tst_responses.moc"
