// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/Batch.h>
#include <QtOpenAi/Core/CreateBatchRequest.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the Batch types (#20): Batch parsing/round-trip including the
// request counts and the error list, the terminal-state classification the
// poller relies on, the BatchList (ListPage) shape, and the create-request body.
class TestBatches : public QObject
{
    Q_OBJECT
private slots:
    void parsesBatch();
    void batchRoundTrip();
    void parsesErrors();
    void reportsTerminalStatus_data();
    void reportsTerminalStatus();
    void parsesBatchList();
    void createRequestSerialisesBody();
    void createRequestOmitsUnsetFields();
};

void TestBatches::parsesBatch()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("batch_abc123")},
            {QStringLiteral("object"), QStringLiteral("batch")},
            {QStringLiteral("endpoint"), QStringLiteral("/v1/chat/completions")},
            {QStringLiteral("input_file_id"), QStringLiteral("file-in")},
            {QStringLiteral("completion_window"), QStringLiteral("24h")},
            {QStringLiteral("status"), QStringLiteral("completed")},
            {QStringLiteral("output_file_id"), QStringLiteral("file-out")},
            {QStringLiteral("error_file_id"), QStringLiteral("file-err")},
            {QStringLiteral("created_at"), 1711471533},
            {QStringLiteral("in_progress_at"), 1711471538},
            {QStringLiteral("expires_at"), 1711557933},
            {QStringLiteral("finalizing_at"), 1711493133},
            {QStringLiteral("completed_at"), 1711493163},
            {QStringLiteral("request_counts"), QJsonObject {{QStringLiteral("total"), 100},
                                                            {QStringLiteral("completed"), 95},
                                                            {QStringLiteral("failed"), 5}}},
            {QStringLiteral("metadata"),
             QJsonObject {{QStringLiteral("job"), QStringLiteral("nightly")}}},
    };

    const Batch batch = Batch::fromJson(json);
    QCOMPARE(batch.id(), QStringLiteral("batch_abc123"));
    QCOMPARE(batch.object(), QStringLiteral("batch"));
    QCOMPARE(batch.endpoint(), QStringLiteral("/v1/chat/completions"));
    QCOMPARE(batch.inputFileId(), QStringLiteral("file-in"));
    QCOMPARE(batch.completionWindow(), QStringLiteral("24h"));
    QCOMPARE(batch.status(), BatchStatus::Completed);
    QCOMPARE(batch.outputFileId(), QStringLiteral("file-out"));
    QCOMPARE(batch.errorFileId(), QStringLiteral("file-err"));
    QCOMPARE(batch.createdAt(), Q_INT64_C(1711471533));
    QCOMPARE(batch.inProgressAt(), Q_INT64_C(1711471538));
    QCOMPARE(batch.expiresAt(), Q_INT64_C(1711557933));
    QCOMPARE(batch.finalizingAt(), Q_INT64_C(1711493133));
    QCOMPARE(batch.completedAt(), Q_INT64_C(1711493163));
    // Absent timestamps stay 0 rather than being invented.
    QCOMPARE(batch.failedAt(), Q_INT64_C(0));
    QCOMPARE(batch.cancelledAt(), Q_INT64_C(0));
    QCOMPARE(batch.requestCounts().total, 100);
    QCOMPARE(batch.requestCounts().completed, 95);
    QCOMPARE(batch.requestCounts().failed, 5);
    QCOMPARE(batch.metadata().value(QStringLiteral("job")).toString(), QStringLiteral("nightly"));
    QVERIFY(batch.errors().isEmpty());
}

void TestBatches::batchRoundTrip()
{
    Batch batch;
    batch.setId(QStringLiteral("batch_1"));
    batch.setObject(QStringLiteral("batch"));
    batch.setEndpoint(QStringLiteral("/v1/embeddings"));
    batch.setInputFileId(QStringLiteral("file-in"));
    batch.setCompletionWindow(QStringLiteral("24h"));
    batch.setStatus(BatchStatus::Cancelling);
    batch.setOutputFileId(QStringLiteral("file-out"));
    batch.setErrorFileId(QStringLiteral("file-err"));
    batch.setCreatedAt(1700000000);
    batch.setInProgressAt(1700000001);
    batch.setExpiresAt(1700086400);
    batch.setFinalizingAt(1700000002);
    batch.setCompletedAt(1700000003);
    batch.setFailedAt(1700000004);
    batch.setExpiredAt(1700000005);
    batch.setCancellingAt(1700000006);
    batch.setCancelledAt(1700000007);
    batch.setRequestCounts({7, 5, 2});
    batch.setMetadata(QJsonObject {{QStringLiteral("k"), QStringLiteral("v")}});
    batch.setErrors({BatchError {QStringLiteral("invalid_json_line"),
                                 QStringLiteral("could not parse"), QStringLiteral("body"), 12}});

    QCOMPARE(Batch::fromJson(batch.toJson()), batch);
}

void TestBatches::parsesErrors()
{
    // Input validation problems arrive as a `list` object nested in `errors`.
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("batch_1")},
            {QStringLiteral("status"), QStringLiteral("failed")},
            {QStringLiteral("errors"),
             QJsonObject {{QStringLiteral("object"), QStringLiteral("list")},
                          {QStringLiteral("data"),
                           QJsonArray {QJsonObject {
                                   {QStringLiteral("code"), QStringLiteral("invalid_json_line")},
                                   {QStringLiteral("message"), QStringLiteral("could not parse")},
                                   {QStringLiteral("param"), QStringLiteral("body")},
                                   {QStringLiteral("line"), 12}}}}}},
    };

    const Batch batch = Batch::fromJson(json);
    QCOMPARE(batch.status(), BatchStatus::Failed);
    QCOMPARE(batch.errors().size(), 1);
    QCOMPARE(batch.errors().first().code, QStringLiteral("invalid_json_line"));
    QCOMPARE(batch.errors().first().message, QStringLiteral("could not parse"));
    QCOMPARE(batch.errors().first().param, QStringLiteral("body"));
    QCOMPARE(batch.errors().first().line, 12);
}

void TestBatches::reportsTerminalStatus_data()
{
    QTest::addColumn<QString>("wireStatus");
    QTest::addColumn<bool>("terminal");

    QTest::newRow("validating") << QStringLiteral("validating") << false;
    QTest::newRow("in_progress") << QStringLiteral("in_progress") << false;
    QTest::newRow("finalizing") << QStringLiteral("finalizing") << false;
    QTest::newRow("cancelling") << QStringLiteral("cancelling") << false;
    QTest::newRow("completed") << QStringLiteral("completed") << true;
    QTest::newRow("failed") << QStringLiteral("failed") << true;
    QTest::newRow("expired") << QStringLiteral("expired") << true;
    QTest::newRow("cancelled") << QStringLiteral("cancelled") << true;
    // An unknown value decodes to the initial state, so polling continues.
    QTest::newRow("unknown") << QStringLiteral("something_new") << false;
}

void TestBatches::reportsTerminalStatus()
{
    QFETCH(QString, wireStatus);
    QFETCH(bool, terminal);

    const Batch batch = Batch::fromJson(QJsonObject {{QStringLiteral("status"), wireStatus}});
    QCOMPARE(batch.isTerminal(), terminal);
}

void TestBatches::parsesBatchList()
{
    const QJsonObject json {
            {QStringLiteral("object"), QStringLiteral("list")},
            {QStringLiteral("data"),
             QJsonArray {QJsonObject {{QStringLiteral("id"), QStringLiteral("batch_1")},
                                      {QStringLiteral("status"), QStringLiteral("completed")}},
                         QJsonObject {{QStringLiteral("id"), QStringLiteral("batch_2")},
                                      {QStringLiteral("status"), QStringLiteral("in_progress")}}}},
            {QStringLiteral("first_id"), QStringLiteral("batch_1")},
            {QStringLiteral("last_id"), QStringLiteral("batch_2")},
            {QStringLiteral("has_more"), true},
    };

    const BatchList list = BatchList::fromJson(json);
    QCOMPARE(list.size(), 2);
    QCOMPARE(list.data.at(1).status(), BatchStatus::InProgress);
    QCOMPARE(list.firstId, QStringLiteral("batch_1"));
    QVERIFY(list.hasMore);
    QCOMPARE(BatchList::fromJson(list.toJson()), list);
}

void TestBatches::createRequestSerialisesBody()
{
    CreateBatchRequest request(QStringLiteral("file-in"), QStringLiteral("/v1/chat/completions"));
    request.setCompletionWindow(QStringLiteral("24h"));
    request.setMetadata(QJsonObject {{QStringLiteral("job"), QStringLiteral("nightly")}});
    request.setOutputExpiresAfter(QStringLiteral("created_at"), 3600);

    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("input_file_id")).toString(), QStringLiteral("file-in"));
    QCOMPARE(json.value(QStringLiteral("endpoint")).toString(),
             QStringLiteral("/v1/chat/completions"));
    QCOMPARE(json.value(QStringLiteral("completion_window")).toString(), QStringLiteral("24h"));
    QCOMPARE(json.value(QStringLiteral("metadata"))
                     .toObject()
                     .value(QStringLiteral("job"))
                     .toString(),
             QStringLiteral("nightly"));
    const QJsonObject expires = json.value(QStringLiteral("output_expires_after")).toObject();
    QCOMPARE(expires.value(QStringLiteral("anchor")).toString(), QStringLiteral("created_at"));
    QCOMPARE(expires.value(QStringLiteral("seconds")).toInt(), 3600);
}

void TestBatches::createRequestOmitsUnsetFields()
{
    // The default completion window is the only one the API accepts today, so
    // it is pre-set; everything optional stays out of the body.
    const CreateBatchRequest request(QStringLiteral("file-in"), QStringLiteral("/v1/embeddings"));

    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("completion_window")).toString(), QStringLiteral("24h"));
    QVERIFY(!json.contains(QStringLiteral("metadata")));
    QVERIFY(!json.contains(QStringLiteral("output_expires_after")));
}

QTEST_MAIN(TestBatches)
#include "tst_batches.moc"
