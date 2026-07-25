// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/CreateVectorStoreRequest.h>
#include <QtOpenAi/Core/VectorStore.h>
#include <QtOpenAi/Core/VectorStoreFile.h>
#include <QtOpenAi/Core/VectorStoreSearch.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the Vector Stores types (#18): status mapping, store/file/batch
// parsing & round-trips, the create/modify body, and the search request and its
// result page (including the string-vs-array `query` form).
class TestVectorStores : public QObject
{
    Q_OBJECT
private slots:
    void statusStringMapping();
    void statusFromUnknownDefaultsToInProgress();
    void parsesVectorStore();
    void vectorStoreRoundTrip();
    void parsesVectorStoreList();
    void parsesVectorStoreFile();
    void parsesFailedVectorStoreFile();
    void vectorStoreFileRoundTrip();
    void parsesFileBatch();
    void createRequestJsonBody();
    void createRequestOmitsUnsetFields();
    void searchRequestSendsSingleQueryAsString();
    void searchRequestSendsMultipleQueriesAsArray();
    void parsesSearchPage();
    void parsesFileContentPage();
};

static QJsonObject parse(const QJsonObject &json)
{
    // Round-trip through the wire format to prove the body survives encoding.
    return QJsonDocument::fromJson(QJsonDocument(json).toJson()).object();
}

void TestVectorStores::statusStringMapping()
{
    QCOMPARE(vectorStoreStatusToString(VectorStoreStatus::InProgress),
             QStringLiteral("in_progress"));
    QCOMPARE(vectorStoreStatusToString(VectorStoreStatus::Completed), QStringLiteral("completed"));
    QCOMPARE(vectorStoreStatusToString(VectorStoreStatus::Expired), QStringLiteral("expired"));
    QCOMPARE(vectorStoreStatusFromString(QStringLiteral("expired")), VectorStoreStatus::Expired);

    QCOMPARE(vectorStoreFileStatusToString(VectorStoreFileStatus::Cancelled),
             QStringLiteral("cancelled"));
    QCOMPARE(vectorStoreFileStatusFromString(QStringLiteral("failed")),
             VectorStoreFileStatus::Failed);
}

void TestVectorStores::statusFromUnknownDefaultsToInProgress()
{
    QCOMPARE(vectorStoreStatusFromString(QStringLiteral("something_new")),
             VectorStoreStatus::InProgress);
    QCOMPARE(vectorStoreFileStatusFromString(QString()), VectorStoreFileStatus::InProgress);
}

void TestVectorStores::parsesVectorStore()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("vs_abc")},
            {QStringLiteral("object"), QStringLiteral("vector_store")},
            {QStringLiteral("created_at"), 1698107661},
            {QStringLiteral("name"), QStringLiteral("Support FAQ")},
            {QStringLiteral("usage_bytes"), 139920},
            {QStringLiteral("file_counts"), QJsonObject {{QStringLiteral("in_progress"), 1},
                                                         {QStringLiteral("completed"), 3},
                                                         {QStringLiteral("cancelled"), 0},
                                                         {QStringLiteral("failed"), 1},
                                                         {QStringLiteral("total"), 5}}},
            {QStringLiteral("status"), QStringLiteral("completed")},
            {QStringLiteral("expires_after"),
             QJsonObject {{QStringLiteral("anchor"), QStringLiteral("last_active_at")},
                          {QStringLiteral("days"), 7}}},
            {QStringLiteral("last_active_at"), 1698107661},
            {QStringLiteral("metadata"),
             QJsonObject {{QStringLiteral("team"), QStringLiteral("cs")}}},
    };

    const VectorStore store = VectorStore::fromJson(json);
    QCOMPARE(store.id(), QStringLiteral("vs_abc"));
    QCOMPARE(store.name(), QStringLiteral("Support FAQ"));
    QCOMPARE(store.usageBytes(), Q_INT64_C(139920));
    QCOMPARE(store.status(), VectorStoreStatus::Completed);
    QCOMPARE(store.fileCounts().completed, 3);
    QCOMPARE(store.fileCounts().total, 5);
    QCOMPARE(store.expiresAfterAnchor(), QStringLiteral("last_active_at"));
    QCOMPARE(store.expiresAfterDays(), 7);
    QCOMPARE(store.lastActiveAt(), Q_INT64_C(1698107661));
    QCOMPARE(store.metadata().value(QStringLiteral("team")).toString(), QStringLiteral("cs"));
}

void TestVectorStores::vectorStoreRoundTrip()
{
    VectorStoreFileCounts counts;
    counts.completed = 2;
    counts.total = 2;

    VectorStore store;
    store.setId(QStringLiteral("vs_1"));
    store.setObject(QStringLiteral("vector_store"));
    store.setCreatedAt(1700000000);
    store.setName(QStringLiteral("docs"));
    store.setUsageBytes(4096);
    store.setFileCounts(counts);
    store.setStatus(VectorStoreStatus::Completed);
    store.setExpiresAfter(QStringLiteral("last_active_at"), 30);
    store.setExpiresAt(1700003600);
    store.setLastActiveAt(1700000500);
    store.setMetadata(QJsonObject {{QStringLiteral("env"), QStringLiteral("prod")}});

    QCOMPARE(VectorStore::fromJson(store.toJson()), store);
}

void TestVectorStores::parsesVectorStoreList()
{
    const QJsonObject json {
            {QStringLiteral("object"), QStringLiteral("list")},
            {QStringLiteral("data"),
             QJsonArray {QJsonObject {{QStringLiteral("id"), QStringLiteral("vs_1")}},
                         QJsonObject {{QStringLiteral("id"), QStringLiteral("vs_2")}}}},
            {QStringLiteral("first_id"), QStringLiteral("vs_1")},
            {QStringLiteral("last_id"), QStringLiteral("vs_2")},
            {QStringLiteral("has_more"), false},
    };

    const VectorStoreList list = VectorStoreList::fromJson(json);
    QCOMPARE(list.size(), 2);
    QCOMPARE(list.data.at(1).id(), QStringLiteral("vs_2"));
    QCOMPARE(VectorStoreList::fromJson(list.toJson()), list);
}

void TestVectorStores::parsesVectorStoreFile()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("file-abc")},
            {QStringLiteral("object"), QStringLiteral("vector_store.file")},
            {QStringLiteral("usage_bytes"), 1234},
            {QStringLiteral("created_at"), 1698107661},
            {QStringLiteral("vector_store_id"), QStringLiteral("vs_abc")},
            {QStringLiteral("status"), QStringLiteral("completed")},
            {QStringLiteral("chunking_strategy"),
             QJsonObject {{QStringLiteral("type"), QStringLiteral("static")}}},
            {QStringLiteral("attributes"),
             QJsonObject {{QStringLiteral("region"), QStringLiteral("eu")}}},
    };

    const VectorStoreFile file = VectorStoreFile::fromJson(json);
    QCOMPARE(file.id(), QStringLiteral("file-abc"));
    QCOMPARE(file.usageBytes(), Q_INT64_C(1234));
    QCOMPARE(file.vectorStoreId(), QStringLiteral("vs_abc"));
    QCOMPARE(file.status(), VectorStoreFileStatus::Completed);
    QCOMPARE(file.chunkingStrategy().value(QStringLiteral("type")).toString(),
             QStringLiteral("static"));
    QCOMPARE(file.attributes().value(QStringLiteral("region")).toString(), QStringLiteral("eu"));
    QVERIFY(file.lastErrorCode().isEmpty());
}

void TestVectorStores::parsesFailedVectorStoreFile()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("file-bad")},
            {QStringLiteral("status"), QStringLiteral("failed")},
            {QStringLiteral("last_error"),
             QJsonObject {{QStringLiteral("code"), QStringLiteral("unsupported_file")},
                          {QStringLiteral("message"), QStringLiteral("cannot parse")}}},
    };

    const VectorStoreFile file = VectorStoreFile::fromJson(json);
    QCOMPARE(file.status(), VectorStoreFileStatus::Failed);
    QCOMPARE(file.lastErrorCode(), QStringLiteral("unsupported_file"));
    QCOMPARE(file.lastErrorMessage(), QStringLiteral("cannot parse"));
}

void TestVectorStores::vectorStoreFileRoundTrip()
{
    VectorStoreFile file;
    file.setId(QStringLiteral("file-1"));
    file.setObject(QStringLiteral("vector_store.file"));
    file.setUsageBytes(99);
    file.setCreatedAt(1700000000);
    file.setVectorStoreId(QStringLiteral("vs_1"));
    file.setStatus(VectorStoreFileStatus::Failed);
    file.setLastErrorCode(QStringLiteral("server_error"));
    file.setLastErrorMessage(QStringLiteral("boom"));
    file.setChunkingStrategy(QJsonObject {{QStringLiteral("type"), QStringLiteral("auto")}});
    file.setAttributes(QJsonObject {{QStringLiteral("lang"), QStringLiteral("de")}});

    QCOMPARE(VectorStoreFile::fromJson(file.toJson()), file);
}

void TestVectorStores::parsesFileBatch()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("vsfb_1")},
            {QStringLiteral("object"), QStringLiteral("vector_store.files_batch")},
            {QStringLiteral("created_at"), 1699061776},
            {QStringLiteral("vector_store_id"), QStringLiteral("vs_abc")},
            {QStringLiteral("status"), QStringLiteral("in_progress")},
            {QStringLiteral("file_counts"),
             QJsonObject {{QStringLiteral("in_progress"), 2}, {QStringLiteral("total"), 2}}},
    };

    const VectorStoreFileBatch batch = VectorStoreFileBatch::fromJson(json);
    QCOMPARE(batch.id(), QStringLiteral("vsfb_1"));
    QCOMPARE(batch.vectorStoreId(), QStringLiteral("vs_abc"));
    QCOMPARE(batch.status(), VectorStoreFileStatus::InProgress);
    QCOMPARE(batch.fileCounts().inProgress, 2);
    QCOMPARE(VectorStoreFileBatch::fromJson(batch.toJson()), batch);
}

void TestVectorStores::createRequestJsonBody()
{
    CreateVectorStoreRequest request(QStringLiteral("Support FAQ"),
                                     {QStringLiteral("file-1"), QStringLiteral("file-2")});
    request.setExpiresAfter(QStringLiteral("last_active_at"), 7);
    request.setChunkingStrategy(QJsonObject {{QStringLiteral("type"), QStringLiteral("auto")}});
    request.setMetadata(QJsonObject {{QStringLiteral("team"), QStringLiteral("cs")}});

    const QJsonObject json = parse(request.toJson());
    QCOMPARE(json.value(QStringLiteral("name")).toString(), QStringLiteral("Support FAQ"));
    QCOMPARE(json.value(QStringLiteral("file_ids")).toArray().size(), 2);
    QCOMPARE(json.value(QStringLiteral("expires_after"))
                     .toObject()
                     .value(QStringLiteral("days"))
                     .toInt(),
             7);
    QCOMPARE(json.value(QStringLiteral("chunking_strategy"))
                     .toObject()
                     .value(QStringLiteral("type"))
                     .toString(),
             QStringLiteral("auto"));
    QVERIFY(json.contains(QStringLiteral("metadata")));
}

void TestVectorStores::createRequestOmitsUnsetFields()
{
    // A modify call reuses this type and must send only what it changes.
    CreateVectorStoreRequest request;
    request.setName(QStringLiteral("renamed"));

    const QJsonObject json = request.toJson();
    QCOMPARE(json.keys(), QStringList {QStringLiteral("name")});
}

void TestVectorStores::searchRequestSendsSingleQueryAsString()
{
    VectorStoreSearchRequest request(QStringLiteral("how do I reset my password"));
    request.setMaxNumResults(5);
    request.setRewriteQuery(true);
    request.setFilters(QJsonObject {{QStringLiteral("type"), QStringLiteral("eq")},
                                    {QStringLiteral("key"), QStringLiteral("region")},
                                    {QStringLiteral("value"), QStringLiteral("eu")}});

    const QJsonObject json = parse(request.toJson());
    QVERIFY(json.value(QStringLiteral("query")).isString());
    QCOMPARE(json.value(QStringLiteral("query")).toString(),
             QStringLiteral("how do I reset my password"));
    QCOMPARE(json.value(QStringLiteral("max_num_results")).toInt(), 5);
    QCOMPARE(json.value(QStringLiteral("rewrite_query")).toBool(), true);
    QCOMPARE(json.value(QStringLiteral("filters"))
                     .toObject()
                     .value(QStringLiteral("key"))
                     .toString(),
             QStringLiteral("region"));
}

void TestVectorStores::searchRequestSendsMultipleQueriesAsArray()
{
    VectorStoreSearchRequest request;
    request.setQuery(QStringList {QStringLiteral("password"), QStringLiteral("login")});

    const QJsonObject json = parse(request.toJson());
    QVERIFY(json.value(QStringLiteral("query")).isArray());
    QCOMPARE(json.value(QStringLiteral("query")).toArray().size(), 2);
    // Unset options stay out of the body.
    QVERIFY(!json.contains(QStringLiteral("max_num_results")));
    QVERIFY(!json.contains(QStringLiteral("rewrite_query")));
}

void TestVectorStores::parsesSearchPage()
{
    const QJsonObject json {
            {QStringLiteral("object"), QStringLiteral("vector_store.search_results.page")},
            {QStringLiteral("search_query"), QJsonArray {QStringLiteral("password reset")}},
            {QStringLiteral("data"),
             QJsonArray {QJsonObject {
                     {QStringLiteral("file_id"), QStringLiteral("file-1")},
                     {QStringLiteral("filename"), QStringLiteral("faq.md")},
                     {QStringLiteral("score"), 0.92},
                     {QStringLiteral("attributes"),
                      QJsonObject {{QStringLiteral("region"), QStringLiteral("eu")}}},
                     {QStringLiteral("content"),
                      QJsonArray {
                              QJsonObject {{QStringLiteral("type"), QStringLiteral("text")},
                                           {QStringLiteral("text"), QStringLiteral("first")}},
                              QJsonObject {{QStringLiteral("type"), QStringLiteral("text")},
                                           {QStringLiteral("text"), QStringLiteral("second")}}}}}}},
            {QStringLiteral("has_more"), true},
            {QStringLiteral("next_page"), QStringLiteral("page_2")},
    };

    const VectorStoreSearchPage page = VectorStoreSearchPage::fromJson(json);
    QCOMPARE(page.size(), 1);
    QCOMPARE(page.searchQuery, QStringList {QStringLiteral("password reset")});
    QVERIFY(page.hasMore);
    QCOMPARE(page.nextPage, QStringLiteral("page_2"));

    const VectorStoreSearchResult &hit = page.data.first();
    QCOMPARE(hit.fileId(), QStringLiteral("file-1"));
    QCOMPARE(hit.filename(), QStringLiteral("faq.md"));
    QVERIFY(qFuzzyCompare(hit.score(), 0.92));
    QCOMPARE(hit.content().size(), 2);
    // The convenience accessor joins the chunks so callers can feed a model.
    QCOMPARE(hit.text(), QStringLiteral("first\nsecond"));

    QCOMPARE(VectorStoreSearchPage::fromJson(page.toJson()), page);
}

void TestVectorStores::parsesFileContentPage()
{
    const QJsonObject json {
            {QStringLiteral("object"), QStringLiteral("vector_store.file_content.page")},
            {QStringLiteral("data"),
             QJsonArray {QJsonObject {{QStringLiteral("type"), QStringLiteral("text")},
                                      {QStringLiteral("text"), QStringLiteral("chunk one")}}}},
            {QStringLiteral("has_more"), false},
    };

    const VectorStoreFileContentPage page = VectorStoreFileContentPage::fromJson(json);
    QCOMPARE(page.size(), 1);
    QCOMPARE(page.text(), QStringLiteral("chunk one"));
    QVERIFY(!page.hasMore);
    QCOMPARE(VectorStoreFileContentPage::fromJson(page.toJson()), page);
}

QTEST_MAIN(TestVectorStores)
#include "tst_vectorstores.moc"
