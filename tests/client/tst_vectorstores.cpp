// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/StubServer.h"

// Offline stub-server coverage for the Vector Stores endpoints (#18): store
// CRUD, the file sub-resource (attach, list, attributes, detach, content), file
// batches, and search.
class TestVectorStoresClient : public QObject
{
    Q_OBJECT
private slots:
    void createPostsJsonBody();
    void listSendsPaginationQuery();
    void getParsesStore();
    void updatePostsChangedFieldsOnly();
    void deleteIssuesDeleteVerb();
    void createFilePostsFileId();
    void listFilesSendsStatusFilter();
    void updateFileAttributesPostsAttributes();
    void deleteFileIssuesDeleteVerb();
    void getFileContentParsesChunks();
    void createFileBatchPostsFileIds();
    void cancelFileBatchPostsToCancelPath();
    void listBatchFilesUsesBatchPath();
    void searchParsesRankedResults();
};

void TestVectorStoresClient::createPostsJsonBody()
{
    StubServer server(QByteArray(R"({"id":"vs_1","object":"vector_store","name":"docs",)"
                                 R"("status":"in_progress","file_counts":{"total":2}})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateVectorStoreRequest request(QStringLiteral("docs"),
                                     {QStringLiteral("file-1"), QStringLiteral("file-2")});

    VectorStoreReply *reply = client.createVectorStore(request);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/vector_stores "));
    QVERIFY(server.requestBody().contains("\"name\":\"docs\""));
    QVERIFY(server.requestBody().contains("\"file_ids\":[\"file-1\",\"file-2\"]"));
    QCOMPARE(reply->store().id(), QStringLiteral("vs_1"));
    QCOMPARE(reply->store().status(), VectorStoreStatus::InProgress);
    QCOMPARE(reply->store().fileCounts().total, 2);
    delete reply;
}

void TestVectorStoresClient::listSendsPaginationQuery()
{
    StubServer server(QByteArray(R"({"object":"list","data":[{"id":"vs_1"},{"id":"vs_2"}],)"
                                 R"("first_id":"vs_1","last_id":"vs_2","has_more":true})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.limit = 2;
    params.order = QStringLiteral("asc");
    VectorStoreListReply *reply = client.listVectorStores(params);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/vector_stores?"));
    QVERIFY(server.requestLine().contains("limit=2"));
    QVERIFY(server.requestLine().contains("order=asc"));
    QCOMPARE(reply->list().size(), 2);
    QVERIFY(reply->list().hasMore);
    delete reply;
}

void TestVectorStoresClient::getParsesStore()
{
    StubServer server(QByteArray(R"({"id":"vs_1","status":"completed","usage_bytes":4096,)"
                                 R"("file_counts":{"completed":3,"total":3}})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    VectorStoreReply *reply = client.getVectorStore(QStringLiteral("vs_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/vector_stores/vs_1 "));
    QCOMPARE(reply->store().status(), VectorStoreStatus::Completed);
    QCOMPARE(reply->store().usageBytes(), Q_INT64_C(4096));
    delete reply;
}

void TestVectorStoresClient::updatePostsChangedFieldsOnly()
{
    StubServer server(QByteArray(R"({"id":"vs_1","name":"renamed"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateVectorStoreRequest request;
    request.setName(QStringLiteral("renamed"));

    VectorStoreReply *reply = client.updateVectorStore(QStringLiteral("vs_1"), request);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/vector_stores/vs_1 "));
    QCOMPARE(server.requestBody(), QByteArray(R"({"name":"renamed"})"));
    delete reply;
}

void TestVectorStoresClient::deleteIssuesDeleteVerb()
{
    StubServer server(
            QByteArray(R"({"id":"vs_1","object":"vector_store.deleted","deleted":true})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    VectorStoreReply *reply = client.deleteVectorStore(QStringLiteral("vs_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("DELETE /v1/vector_stores/vs_1 "));
    QCOMPARE(reply->store().object(), QStringLiteral("vector_store.deleted"));
    delete reply;
}

void TestVectorStoresClient::createFilePostsFileId()
{
    StubServer server(QByteArray(R"({"id":"file-1","object":"vector_store.file",)"
                                 R"("vector_store_id":"vs_1","status":"in_progress"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    VectorStoreFileReply *reply = client.createVectorStoreFile(
            QStringLiteral("vs_1"), QStringLiteral("file-1"),
            QJsonObject {{QStringLiteral("type"), QStringLiteral("auto")}},
            QJsonObject {{QStringLiteral("region"), QStringLiteral("eu")}});
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/vector_stores/vs_1/files "));
    QVERIFY(server.requestBody().contains("\"file_id\":\"file-1\""));
    QVERIFY(server.requestBody().contains("\"chunking_strategy\":{\"type\":\"auto\"}"));
    QVERIFY(server.requestBody().contains("\"attributes\":{\"region\":\"eu\"}"));
    QCOMPARE(reply->file().status(), VectorStoreFileStatus::InProgress);
    delete reply;
}

void TestVectorStoresClient::listFilesSendsStatusFilter()
{
    StubServer server(QByteArray(R"({"object":"list","data":[{"id":"file-1"}],)"
                                 R"("has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    VectorStoreFileListReply *reply
            = client.listVectorStoreFiles(QStringLiteral("vs_1"), {}, QStringLiteral("completed"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/vector_stores/vs_1/files?"));
    QVERIFY(server.requestLine().contains("filter=completed"));
    QCOMPARE(reply->list().size(), 1);
    delete reply;
}

void TestVectorStoresClient::updateFileAttributesPostsAttributes()
{
    StubServer server(QByteArray(R"({"id":"file-1","attributes":{"region":"us"}})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    VectorStoreFileReply *reply = client.updateVectorStoreFileAttributes(
            QStringLiteral("vs_1"), QStringLiteral("file-1"),
            QJsonObject {{QStringLiteral("region"), QStringLiteral("us")}});
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/vector_stores/vs_1/files/file-1 "));
    QCOMPARE(server.requestBody(), QByteArray(R"({"attributes":{"region":"us"}})"));
    QCOMPARE(reply->file().attributes().value(QStringLiteral("region")).toString(),
             QStringLiteral("us"));
    delete reply;
}

void TestVectorStoresClient::deleteFileIssuesDeleteVerb()
{
    StubServer server(
            QByteArray(R"({"id":"file-1","object":"vector_store.file.deleted","deleted":true})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    VectorStoreFileReply *reply
            = client.deleteVectorStoreFile(QStringLiteral("vs_1"), QStringLiteral("file-1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("DELETE /v1/vector_stores/vs_1/files/file-1 "));
    QCOMPARE(reply->file().object(), QStringLiteral("vector_store.file.deleted"));
    delete reply;
}

void TestVectorStoresClient::getFileContentParsesChunks()
{
    StubServer server(QByteArray(R"({"object":"vector_store.file_content.page",)"
                                 R"("data":[{"type":"text","text":"alpha"},)"
                                 R"({"type":"text","text":"beta"}],"has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    VectorStoreFileContentReply *reply
            = client.getVectorStoreFileContent(QStringLiteral("vs_1"), QStringLiteral("file-1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/vector_stores/vs_1/files/file-1/content "));
    QCOMPARE(reply->page().size(), 2);
    QCOMPARE(reply->page().text(), QStringLiteral("alpha\nbeta"));
    delete reply;
}

void TestVectorStoresClient::createFileBatchPostsFileIds()
{
    StubServer server(QByteArray(R"({"id":"vsfb_1","object":"vector_store.files_batch",)"
                                 R"("vector_store_id":"vs_1","status":"in_progress",)"
                                 R"("file_counts":{"in_progress":2,"total":2}})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    VectorStoreFileBatchReply *reply = client.createVectorStoreFileBatch(
            QStringLiteral("vs_1"), {QStringLiteral("file-1"), QStringLiteral("file-2")});
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/vector_stores/vs_1/file_batches "));
    QVERIFY(server.requestBody().contains("\"file_ids\":[\"file-1\",\"file-2\"]"));
    QCOMPARE(reply->batch().id(), QStringLiteral("vsfb_1"));
    QCOMPARE(reply->batch().fileCounts().total, 2);
    delete reply;
}

void TestVectorStoresClient::cancelFileBatchPostsToCancelPath()
{
    StubServer server(QByteArray(R"({"id":"vsfb_1","status":"cancelled"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    VectorStoreFileBatchReply *reply
            = client.cancelVectorStoreFileBatch(QStringLiteral("vs_1"), QStringLiteral("vsfb_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith(
            "POST /v1/vector_stores/vs_1/file_batches/vsfb_1/cancel "));
    QCOMPARE(reply->batch().status(), VectorStoreFileStatus::Cancelled);
    delete reply;
}

void TestVectorStoresClient::listBatchFilesUsesBatchPath()
{
    StubServer server(QByteArray(R"({"object":"list","data":[{"id":"file-1"}],)"
                                 R"("has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.limit = 10;
    VectorStoreFileListReply *reply = client.listVectorStoreFileBatchFiles(
            QStringLiteral("vs_1"), QStringLiteral("vsfb_1"), params);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith(
            "GET /v1/vector_stores/vs_1/file_batches/vsfb_1/files?"));
    QVERIFY(server.requestLine().contains("limit=10"));
    delete reply;
}

void TestVectorStoresClient::searchParsesRankedResults()
{
    StubServer server(QByteArray(R"({"object":"vector_store.search_results.page",)"
                                 R"("search_query":["password reset"],)"
                                 R"("data":[{"file_id":"file-1","filename":"faq.md",)"
                                 R"("score":0.9,"content":[{"type":"text","text":"reset it"}]},)"
                                 R"({"file_id":"file-2","filename":"guide.md","score":0.4,)"
                                 R"("content":[]}],"has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    VectorStoreSearchRequest request(QStringLiteral("password reset"));
    request.setMaxNumResults(2);

    VectorStoreSearchReply *reply = client.searchVectorStore(QStringLiteral("vs_1"), request);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/vector_stores/vs_1/search "));
    QVERIFY(server.requestBody().contains("\"query\":\"password reset\""));
    QVERIFY(server.requestBody().contains("\"max_num_results\":2"));
    QCOMPARE(reply->page().size(), 2);
    // Ranked best-first, as the server returned them.
    QCOMPARE(reply->page().data.first().fileId(), QStringLiteral("file-1"));
    QVERIFY(reply->page().data.first().score() > reply->page().data.last().score());
    QCOMPARE(reply->page().data.first().text(), QStringLiteral("reset it"));
    delete reply;
}

QTEST_MAIN(TestVectorStoresClient)
#include "tst_vectorstores.moc"
