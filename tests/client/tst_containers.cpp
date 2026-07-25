// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/StubServer.h"

// Offline stub-server coverage for the Containers endpoints (#19): container
// CRUD plus the file sub-resource, including both ways of adding a file (a
// multipart upload and a reference to an existing Files-API file) and the binary
// content download.
class TestContainersClient : public QObject
{
    Q_OBJECT
private slots:
    void createPostsJsonBody();
    void listSendsPaginationQuery();
    void getParsesContainer();
    void deleteIssuesDeleteVerb();
    void uploadFilePostsMultipart();
    void attachFilePostsFileId();
    void listFilesUsesFilesPath();
    void getFileParsesPath();
    void deleteFileIssuesDeleteVerb();
    void downloadsContentVerbatim();
};

void TestContainersClient::createPostsJsonBody()
{
    StubServer server(QByteArray(R"({"id":"cntr_1","object":"container","name":"analysis",)"
                                 R"("status":"running"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateContainerRequest request(QStringLiteral("analysis"), {QStringLiteral("file-1")});
    request.setExpiresAfter(QStringLiteral("last_active_at"), 20);

    ContainerReply *reply = client.createContainer(request);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/containers "));
    QVERIFY(server.requestBody().contains("\"name\":\"analysis\""));
    QVERIFY(server.requestBody().contains("\"file_ids\":[\"file-1\"]"));
    QVERIFY(server.requestBody().contains("\"minutes\":20"));
    QCOMPARE(reply->container().id(), QStringLiteral("cntr_1"));
    QCOMPARE(reply->container().status(), QStringLiteral("running"));
    delete reply;
}

void TestContainersClient::listSendsPaginationQuery()
{
    StubServer server(QByteArray(R"({"object":"list","data":[{"id":"cntr_1"},{"id":"cntr_2"}],)"
                                 R"("has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.limit = 5;
    ContainerListReply *reply = client.listContainers(params);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/containers?"));
    QVERIFY(server.requestLine().contains("limit=5"));
    QCOMPARE(reply->list().size(), 2);
    delete reply;
}

void TestContainersClient::getParsesContainer()
{
    StubServer server(QByteArray(R"({"id":"cntr_1","status":"running","created_at":1747844794})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ContainerReply *reply = client.getContainer(QStringLiteral("cntr_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/containers/cntr_1 "));
    QCOMPARE(reply->container().createdAt(), Q_INT64_C(1747844794));
    delete reply;
}

void TestContainersClient::deleteIssuesDeleteVerb()
{
    StubServer server(QByteArray(R"({"id":"cntr_1","object":"container.deleted","deleted":true})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ContainerReply *reply = client.deleteContainer(QStringLiteral("cntr_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("DELETE /v1/containers/cntr_1 "));
    QCOMPARE(reply->container().object(), QStringLiteral("container.deleted"));
    delete reply;
}

void TestContainersClient::uploadFilePostsMultipart()
{
    StubServer server(QByteArray(R"({"id":"cfile_1","object":"container.file",)"
                                 R"("container_id":"cntr_1","path":"/mnt/data/a.csv",)"
                                 R"("bytes":9,"source":"user"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ContainerFileReply *reply = client.uploadContainerFile(
            QStringLiteral("cntr_1"), QStringLiteral("a.csv"), QByteArray("a,b\n1,2\n"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/containers/cntr_1/files "));
    QVERIFY(server.requestHeaders().toLower().contains("content-type: multipart/form-data;"));
    QVERIFY(server.requestBody().contains("name=\"file\"; filename=\"a.csv\""));
    QVERIFY(server.requestBody().contains("a,b"));
    QCOMPARE(reply->file().path(), QStringLiteral("/mnt/data/a.csv"));
    delete reply;
}

void TestContainersClient::attachFilePostsFileId()
{
    StubServer server(QByteArray(R"({"id":"cfile_2","container_id":"cntr_1","source":"user"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ContainerFileReply *reply
            = client.attachContainerFile(QStringLiteral("cntr_1"), QStringLiteral("file-1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/containers/cntr_1/files "));
    // The reference form is plain JSON, not a multipart upload.
    QCOMPARE(server.requestBody(), QByteArray(R"({"file_id":"file-1"})"));
    QCOMPARE(reply->file().id(), QStringLiteral("cfile_2"));
    delete reply;
}

void TestContainersClient::listFilesUsesFilesPath()
{
    StubServer server(QByteArray(R"({"object":"list","data":[{"id":"cfile_1"}],)"
                                 R"("has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ContainerFileListReply *reply = client.listContainerFiles(QStringLiteral("cntr_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/containers/cntr_1/files "));
    QCOMPARE(reply->list().size(), 1);
    delete reply;
}

void TestContainersClient::getFileParsesPath()
{
    StubServer server(QByteArray(R"({"id":"cfile_1","path":"/mnt/data/a.csv","bytes":8})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ContainerFileReply *reply
            = client.getContainerFile(QStringLiteral("cntr_1"), QStringLiteral("cfile_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/containers/cntr_1/files/cfile_1 "));
    QCOMPARE(reply->file().bytes(), Q_INT64_C(8));
    delete reply;
}

void TestContainersClient::deleteFileIssuesDeleteVerb()
{
    StubServer server(
            QByteArray(R"({"id":"cfile_1","object":"container.file.deleted","deleted":true})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ContainerFileReply *reply
            = client.deleteContainerFile(QStringLiteral("cntr_1"), QStringLiteral("cfile_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("DELETE /v1/containers/cntr_1/files/cfile_1 "));
    QCOMPARE(reply->file().object(), QStringLiteral("container.file.deleted"));
    delete reply;
}

void TestContainersClient::downloadsContentVerbatim()
{
    // Canned payload including a NUL byte to prove binary-safe handling.
    const QByteArray content("plot\x00\x89PNG", 9);
    StubServer server(content, QByteArray("application/octet-stream"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    BinaryReply *reply = client.downloadContainerFileContent(QStringLiteral("cntr_1"),
                                                             QStringLiteral("cfile_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/containers/cntr_1/files/cfile_1/content "));
    QCOMPARE(reply->data(), content);
    QCOMPARE(reply->contentType(), QByteArray("application/octet-stream"));
    delete reply;
}

QTEST_MAIN(TestContainersClient)
#include "tst_containers.moc"
