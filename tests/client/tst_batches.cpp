// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

// Offline stub-server coverage for the Batch endpoints (#20): the four REST
// calls plus the poll-until-terminal helper, which is the first user of the
// shared JobPoller engine besides VideoPoller.
class TestBatchesClient : public QObject
{
    Q_OBJECT
private slots:
    void createPostsJsonBody();
    void listSendsPaginationQuery();
    void getParsesBatch();
    void cancelPostsToCancelPath();
    void pollsUntilCompleted();
    void pollStopsOnRequestFailure();
};

void TestBatchesClient::createPostsJsonBody()
{
    StubServer server(
            QByteArray(R"({"id":"batch_1","object":"batch","status":"validating",)"
                       R"("endpoint":"/v1/chat/completions","input_file_id":"file-in"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateBatchRequest request(QStringLiteral("file-in"), QStringLiteral("/v1/chat/completions"));
    request.setMetadata(QJsonObject {{QStringLiteral("job"), QStringLiteral("nightly")}});

    const auto reply = awaited(client.createBatch(request));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/batches "));
    QVERIFY(server.requestBody().contains("\"input_file_id\":\"file-in\""));
    QVERIFY(server.requestBody().contains("\"completion_window\":\"24h\""));
    QVERIFY(server.requestBody().contains("\"job\":\"nightly\""));
    QCOMPARE(reply->batch().id(), QStringLiteral("batch_1"));
    QCOMPARE(reply->batch().status(), BatchStatus::Validating);
}

void TestBatchesClient::listSendsPaginationQuery()
{
    StubServer server(QByteArray(R"({"object":"list","data":[{"id":"batch_1"},{"id":"batch_2"}],)"
                                 R"("has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.limit = 5;
    params.after = QStringLiteral("batch_0");
    const auto reply = awaited(client.listBatches(params));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/batches?"));
    QVERIFY(server.requestLine().contains("limit=5"));
    QVERIFY(server.requestLine().contains("after=batch_0"));
    QCOMPARE(reply->list().size(), 2);
}

void TestBatchesClient::getParsesBatch()
{
    StubServer server(QByteArray(R"({"id":"batch_1","status":"completed",)"
                                 R"("output_file_id":"file-out","created_at":1711471533,)"
                                 R"("request_counts":{"total":3,"completed":3,"failed":0}})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.getBatch(QStringLiteral("batch_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/batches/batch_1 "));
    QCOMPARE(reply->batch().outputFileId(), QStringLiteral("file-out"));
    QCOMPARE(reply->batch().createdAt(), Q_INT64_C(1711471533));
    QCOMPARE(reply->batch().requestCounts().completed, 3);
    QVERIFY(reply->batch().isTerminal());
}

void TestBatchesClient::cancelPostsToCancelPath()
{
    StubServer server(QByteArray(R"({"id":"batch_1","status":"cancelling"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.cancelBatch(QStringLiteral("batch_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/batches/batch_1/cancel "));
    QCOMPARE(reply->batch().status(), BatchStatus::Cancelling);
    // Cancelling is not terminal — the batch settles on `cancelled` later.
    QVERIFY(!reply->batch().isTerminal());
}

void TestBatchesClient::pollsUntilCompleted()
{
    StubServer server(QList<StubServer::Response> {
            {R"({"id":"batch_1","status":"validating"})"},
            {R"({"id":"batch_1","status":"in_progress",)"
             R"("request_counts":{"total":2,"completed":1,"failed":0}})"},
            {R"({"id":"batch_1","status":"completed","output_file_id":"file-out",)"
             R"("request_counts":{"total":2,"completed":2,"failed":0}})"},
    });
    Client client(server.baseUrl(), QStringLiteral("k"));

    BatchPoller *poller = client.pollBatch(QStringLiteral("batch_1"), 10);
    poller->setAutoDelete(false);

    QList<BatchStatus> observed;
    connect(poller, &BatchPoller::progressed, this,
            [&observed](const Batch &batch) { observed.append(batch.status()); });
    QSignalSpy completedSpy(poller, &BatchPoller::completed);

    poller->start();
    QVERIFY(completedSpy.wait(5000));

    QCOMPARE(completedSpy.count(), 1);
    QVERIFY(poller->isFinished());
    QVERIFY(!poller->isPolling());
    QCOMPARE(poller->batch().status(), BatchStatus::Completed);
    QCOMPARE(poller->batch().outputFileId(), QStringLiteral("file-out"));
    // Saw the two transient states plus the terminal one, in order.
    QCOMPARE(observed.size(), 3);
    QCOMPARE(observed.first(), BatchStatus::Validating);
    QCOMPARE(observed.last(), BatchStatus::Completed);
    QVERIFY(server.requestLines().first().startsWith("GET /v1/batches/batch_1 "));
    delete poller;
}

void TestBatchesClient::pollStopsOnRequestFailure()
{
    // 400 rather than a 5xx: the default retry policy would otherwise re-issue
    // the request with backoff and drag the test out.
    StubServer server(400, QByteArray(R"({"error":{"message":"no such batch"}})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    BatchPoller *poller = client.pollBatch(QStringLiteral("batch_x"), 10);
    poller->setAutoDelete(false);
    QSignalSpy failedSpy(poller, &BatchPoller::failed);

    poller->start();
    QVERIFY(failedSpy.wait(5000));

    QCOMPARE(failedSpy.count(), 1);
    QVERIFY(poller->isFinished());
    QVERIFY(!poller->isPolling());
    delete poller;
}

QTEST_MAIN(TestBatchesClient)
#include "tst_batches.moc"
