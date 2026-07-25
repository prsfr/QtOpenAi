// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtCore/QBuffer>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/StubServer.h"

// Offline stub-server coverage for the Uploads endpoints (#17): the four REST
// calls of the start → parts → complete flow, plus the ChunkedUploader helper
// that drives the whole flow and splits a payload into the expected parts.
class TestUploadsClient : public QObject
{
    Q_OBJECT
private slots:
    void createPostsJsonBody();
    void addPartUploadsMultipart();
    void completePostsPartIds();
    void completeSendsOptionalMd5();
    void cancelPostsToCancelPath();
    void chunkedUploaderSplitsPayloadAndCompletes();
    void chunkedUploaderReportsFailure();
};

void TestUploadsClient::createPostsJsonBody()
{
    StubServer server(QByteArray(R"({"id":"upload_1","object":"upload","status":"pending",)"
                                 R"("filename":"big.jsonl","bytes":100,"purpose":"fine-tune"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateUploadRequest request(QStringLiteral("big.jsonl"), QStringLiteral("fine-tune"), 100,
                                QStringLiteral("text/jsonl"));

    UploadReply *reply = client.createUpload(request);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/uploads "));
    QVERIFY(server.requestBody().contains("\"filename\":\"big.jsonl\""));
    QVERIFY(server.requestBody().contains("\"mime_type\":\"text/jsonl\""));
    QCOMPARE(reply->upload().id(), QStringLiteral("upload_1"));
    QCOMPARE(reply->upload().status(), UploadStatus::Pending);
    QVERIFY(!reply->upload().isTerminal());
    delete reply;
}

void TestUploadsClient::addPartUploadsMultipart()
{
    StubServer server(QByteArray(R"({"id":"part_1","object":"upload.part",)"
                                 R"("upload_id":"upload_1","created_at":1700000000})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    UploadPartReply *reply
            = client.addUploadPart(QStringLiteral("upload_1"), QByteArray("chunk-bytes"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/uploads/upload_1/parts "));
    QVERIFY(server.requestHeaders().toLower().contains("content-type: multipart/form-data;"));
    QVERIFY(server.requestBody().contains("name=\"data\""));
    QVERIFY(server.requestBody().contains("chunk-bytes"));
    QCOMPARE(reply->part().id(), QStringLiteral("part_1"));
    QCOMPARE(reply->part().uploadId(), QStringLiteral("upload_1"));
    delete reply;
}

void TestUploadsClient::completePostsPartIds()
{
    StubServer server(QByteArray(R"({"id":"upload_1","status":"completed",)"
                                 R"("file":{"id":"file-1","purpose":"fine-tune"}})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    UploadReply *reply = client.completeUpload(
            QStringLiteral("upload_1"), {QStringLiteral("part_1"), QStringLiteral("part_2")});
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/uploads/upload_1/complete "));
    QVERIFY(server.requestBody().contains("\"part_ids\":[\"part_1\",\"part_2\"]"));
    QVERIFY(!server.requestBody().contains("md5"));
    QCOMPARE(reply->upload().status(), UploadStatus::Completed);
    QVERIFY(reply->upload().file().has_value());
    QCOMPARE(reply->upload().file()->id(), QStringLiteral("file-1"));
    delete reply;
}

void TestUploadsClient::completeSendsOptionalMd5()
{
    StubServer server(QByteArray(R"({"id":"upload_1","status":"completed"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    UploadReply *reply = client.completeUpload(
            QStringLiteral("upload_1"), {QStringLiteral("part_1")}, QStringLiteral("d41d8cd9"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestBody().contains("\"md5\":\"d41d8cd9\""));
    delete reply;
}

void TestUploadsClient::cancelPostsToCancelPath()
{
    StubServer server(QByteArray(R"({"id":"upload_1","status":"cancelled"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    UploadReply *reply = client.cancelUpload(QStringLiteral("upload_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/uploads/upload_1/cancel "));
    QCOMPARE(reply->upload().status(), UploadStatus::Cancelled);
    QVERIFY(reply->upload().isTerminal());
    delete reply;
}

void TestUploadsClient::chunkedUploaderSplitsPayloadAndCompletes()
{
    // 10 bytes at a 4-byte chunk size => 3 parts, so the whole flow is
    // create + 3 × part + complete = 5 requests.
    StubServer server(QList<StubServer::Response> {
            {R"({"id":"upload_1","status":"pending","bytes":10})"},
            {R"({"id":"part_1","upload_id":"upload_1"})"},
            {R"({"id":"part_2","upload_id":"upload_1"})"},
            {R"({"id":"part_3","upload_id":"upload_1"})"},
            {R"({"id":"upload_1","status":"completed","file":{"id":"file-1"}})"}});
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateUploadRequest request(QStringLiteral("big.bin"), QStringLiteral("batch"), 0,
                                QStringLiteral("application/octet-stream"));

    ChunkedUploader *uploader
            = client.uploadInChunks(request, QByteArray("0123456789"), /*chunkSize=*/4);
    uploader->setAutoDelete(false);

    QList<QPair<qint64, qint64>> progress;
    connect(uploader, &ChunkedUploader::progressed, this,
            [&progress](qint64 sent, qint64 total) { progress.append({sent, total}); });

    Upload completed;
    connect(uploader, &ChunkedUploader::completed, this,
            [&completed](const Upload &upload) { completed = upload; });

    uploader->start();
    QVERIFY(QTest::qWaitFor([uploader] { return uploader->isFinished(); }, 5000));

    QCOMPARE(server.requestCount(), 5);
    QVERIFY(server.requestLines().at(0).startsWith("POST /v1/uploads "));
    QVERIFY(server.requestLines().at(1).startsWith("POST /v1/uploads/upload_1/parts "));
    QVERIFY(server.requestLines().at(4).startsWith("POST /v1/uploads/upload_1/complete "));

    // The size is filled in from the payload when the request leaves it at 0.
    QVERIFY(server.requestBodies().at(0).contains("\"bytes\":10"));
    // Every part id collected during the run is replayed to /complete, in order.
    QVERIFY(server.requestBodies().at(4).contains(
            "\"part_ids\":[\"part_1\",\"part_2\",\"part_3\"]"));
    // The last chunk is the 2-byte remainder.
    QVERIFY(server.requestBodies().at(3).contains("89"));

    QCOMPARE(progress.size(), 3);
    QCOMPARE(progress.last().first, Q_INT64_C(10));
    QCOMPARE(progress.last().second, Q_INT64_C(10));
    QCOMPARE(completed.status(), UploadStatus::Completed);
    QCOMPARE(completed.file()->id(), QStringLiteral("file-1"));
    delete uploader;
}

void TestUploadsClient::chunkedUploaderReportsFailure()
{
    // The create call already fails, so the helper must surface it and stop.
    // 400 is not in the retryable set, keeping the test free of backoff delays.
    StubServer server(400, QByteArray(R"({"error":{"message":"boom","type":"invalid_request"}})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateUploadRequest request(QStringLiteral("big.bin"), QStringLiteral("batch"), 0,
                                QStringLiteral("application/octet-stream"));

    ChunkedUploader *uploader = client.uploadInChunks(request, QByteArray("payload"), 4);
    uploader->setAutoDelete(false);

    ClientError seen;
    connect(uploader, &ChunkedUploader::failed, this,
            [&seen](const ClientError &error) { seen = error; });

    uploader->start();
    QVERIFY(QTest::qWaitFor([uploader] { return uploader->isFinished(); }, 5000));

    QCOMPARE(seen.message(), QStringLiteral("boom"));
    delete uploader;
}

QTEST_MAIN(TestUploadsClient)
#include "tst_uploads.moc"
