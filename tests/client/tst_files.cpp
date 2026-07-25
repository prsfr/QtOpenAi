// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/StubServer.h"

// Offline stub-server coverage for the Files endpoints (#16): the multipart
// upload, the list query (including the purpose filter), retrieval, deletion and
// the binary content download.
class TestFilesClient : public QObject
{
    Q_OBJECT
private slots:
    void uploadPostsMultipart();
    void listParsesPageAndSendsQuery();
    void getParsesFile();
    void deleteIssuesDeleteVerb();
    void downloadsContentVerbatim();
};

void TestFilesClient::uploadPostsMultipart()
{
    StubServer server(QByteArray(R"({"id":"file-1","object":"file","bytes":8,)"
                                 R"("filename":"data.jsonl","purpose":"fine-tune",)"
                                 R"("status":"processed"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    FileUploadRequest request(QByteArray("{\"a\":1}\n"), QStringLiteral("data.jsonl"),
                              QStringLiteral("fine-tune"));

    FileReply *reply = client.uploadFile(request);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/files "));
    QVERIFY(server.requestHeaders().toLower().contains("content-type: multipart/form-data;"));
    const QByteArray body = server.requestBody();
    QVERIFY(body.contains("name=\"purpose\""));
    QVERIFY(body.contains("fine-tune"));
    QVERIFY(body.contains("name=\"file\"; filename=\"data.jsonl\""));
    QCOMPARE(reply->file().id(), QStringLiteral("file-1"));
    QCOMPARE(reply->file().purpose(), QStringLiteral("fine-tune"));
    delete reply;
}

void TestFilesClient::listParsesPageAndSendsQuery()
{
    StubServer server(QByteArray(R"({"object":"list","data":[{"id":"file-1"},{"id":"file-2"}],)"
                                 R"("first_id":"file-1","last_id":"file-2","has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.limit = 2;
    FileListReply *reply = client.listFiles(params, QStringLiteral("assistants"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/files?"));
    QVERIFY(server.requestLine().contains("limit=2"));
    QVERIFY(server.requestLine().contains("purpose=assistants"));
    QCOMPARE(reply->list().size(), 2);
    QCOMPARE(reply->list().data.at(0).id(), QStringLiteral("file-1"));
    delete reply;
}

void TestFilesClient::getParsesFile()
{
    StubServer server(QByteArray(R"({"id":"file-1","object":"file","bytes":12,)"
                                 R"("created_at":1700000000,"filename":"a.txt"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    FileReply *reply = client.getFile(QStringLiteral("file-1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/files/file-1 "));
    QCOMPARE(reply->file().bytes(), Q_INT64_C(12));
    QCOMPARE(reply->file().filename(), QStringLiteral("a.txt"));
    delete reply;
}

void TestFilesClient::deleteIssuesDeleteVerb()
{
    StubServer server(QByteArray(R"({"id":"file-1","object":"file.deleted","deleted":true})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    FileReply *reply = client.deleteFile(QStringLiteral("file-1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("DELETE /v1/files/file-1 "));
    QCOMPARE(reply->file().object(), QStringLiteral("file.deleted"));
    delete reply;
}

void TestFilesClient::downloadsContentVerbatim()
{
    // Canned payload including a NUL byte to prove binary-safe handling.
    const QByteArray content("line1\n\x00 binary", 14);
    StubServer server(content, QByteArray("application/octet-stream"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    BinaryReply *reply = client.downloadFileContent(QStringLiteral("file-1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/files/file-1/content "));
    QCOMPARE(reply->data(), content);
    QCOMPARE(reply->contentType(), QByteArray("application/octet-stream"));
    delete reply;
}

QTEST_MAIN(TestFilesClient)
#include "tst_files.moc"
