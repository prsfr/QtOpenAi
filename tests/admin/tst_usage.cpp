// SPDX-License-Identifier: MIT
#include <QtOpenAi/Admin/Organization.h>
#include <QtOpenAi/Core/OrganizationCosts.h>
#include <QtOpenAi/Core/OrganizationUsage.h>

#include <QtTest/QtTest>

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Admin;

namespace {

// A completions report grouped by model: two buckets, the second one empty
// because nothing happened in that interval. The server really does send the
// empty bucket, and a caller plotting a chart depends on it.
QByteArray usagePage()
{
    return R"({"object":"page","data":[
        {"object":"bucket","start_time":1730419200,"end_time":1730505600,"results":[
            {"object":"organization.usage.completions.result","input_tokens":1200,
             "output_tokens":340,"input_cached_tokens":800,"input_audio_tokens":0,
             "output_audio_tokens":0,"num_model_requests":7,"project_id":null,
             "user_id":null,"api_key_id":null,"model":"gpt-4o","batch":false}]},
        {"object":"bucket","start_time":1730505600,"end_time":1730592000,"results":[]}],
        "has_more":true,"next_page":"page_AAA"})";
}

QByteArray costsPage()
{
    return R"({"object":"page","data":[
        {"object":"bucket","start_time":1730419200,"end_time":1730505600,"results":[
            {"object":"organization.costs.result","amount":{"value":0.06,"currency":"usd"},
             "line_item":"gpt-4o, input","project_id":"proj_a"}]}],
        "has_more":false,"next_page":null})";
}

} // namespace

// Coverage for the administration usage and cost reports (#99, under #28).
// Offline: every request goes to the local stub server.
class TestUsage : public QObject
{
    Q_OBJECT
private slots:
    void aUsageResultRoundTripsThroughJson();
    void anUnknownCounterSurvivesTheRoundTrip();
    void anUngroupedResultKeepsItsKeysAbsent();
    void aBucketPageRoundTripsThroughJson();
    void aCostResultRoundTripsThroughJson();
    void theQueryPutsEveryBucketParameterOnTheWire();
    void theQueryOmitsWhatTheCallerDidNotSet();
    void usageDecodesThePageAndReachesTheRightEndpoint();
    void everyUsageKindHasItsOwnPath_data();
    void everyUsageKindHasItsOwnPath();
    void costsDecodeTheAmountAndItsCurrency();
};

void TestUsage::aUsageResultRoundTripsThroughJson()
{
    UsageResult result;
    result.setObject(QStringLiteral("organization.usage.completions.result"));
    result.setProjectId(QStringLiteral("proj_a"));
    result.setUserId(QStringLiteral("user_b"));
    result.setApiKeyId(QStringLiteral("key_c"));
    result.setModel(QStringLiteral("gpt-4o"));
    result.setBatch(false);
    result.setMetric(QStringLiteral("input_tokens"), 1200);
    result.setMetric(QStringLiteral("output_tokens"), 340);
    result.setMetric(QStringLiteral("num_model_requests"), 7);

    const UsageResult restored = UsageResult::fromJson(result.toJson());
    QCOMPARE(restored, result);
    QCOMPARE(restored.inputTokens(), qint64(1200));
    QCOMPARE(restored.outputTokens(), qint64(340));
    QCOMPARE(restored.totalTokens(), qint64(1540));
    QCOMPARE(restored.numModelRequests(), qint64(7));
    // A counter this endpoint does not report reads as 0 rather than failing.
    QCOMPARE(restored.characters(), qint64(0));
    QCOMPARE(restored.batch(), std::optional<bool>(false));
}

void TestUsage::anUnknownCounterSurvivesTheRoundTrip()
{
    // The reason the counters are a map: a report that grew a counter after this
    // build shipped must still hand it to the caller.
    const QJsonObject json = QJsonDocument::fromJson(R"({
        "object":"organization.usage.reasoning.result","reasoning_tokens":99,
        "num_model_requests":2,"model":"o5"})")
                                     .object();

    const UsageResult result = UsageResult::fromJson(json);
    QCOMPARE(result.metric(QStringLiteral("reasoning_tokens")), qint64(99));
    QCOMPARE(result.metrics().size(), 2);
    QCOMPARE(result.model(), QStringLiteral("o5"));
    // And it is still there on the way back out.
    QCOMPARE(UsageResult::fromJson(result.toJson()), result);
    QCOMPARE(result.toJson().value(QStringLiteral("reasoning_tokens")).toInt(), 99);
}

void TestUsage::anUngroupedResultKeepsItsKeysAbsent()
{
    // An ungrouped report sends null for every grouping key. Those must not come
    // back out as empty strings, or a bucket total would look like a row for the
    // project whose id is "".
    const QJsonObject json = QJsonDocument::fromJson(R"({
        "object":"organization.usage.embeddings.result","input_tokens":5,
        "num_model_requests":1,"project_id":null,"user_id":null,"api_key_id":null,
        "model":null})")
                                     .object();

    const UsageResult result = UsageResult::fromJson(json);
    QVERIFY(result.projectId().isEmpty());
    QVERIFY(result.model().isEmpty());
    QVERIFY(!result.batch().has_value());

    const QJsonObject out = result.toJson();
    QVERIFY(!out.contains(QStringLiteral("project_id")));
    QVERIFY(!out.contains(QStringLiteral("model")));
    QVERIFY(!out.contains(QStringLiteral("batch")));
    // A zero counter is still a counter and stays.
    QCOMPARE(out.value(QStringLiteral("num_model_requests")).toInt(), 1);
}

void TestUsage::aBucketPageRoundTripsThroughJson()
{
    UsageResult result;
    result.setMetric(QStringLiteral("input_tokens"), 10);

    UsageBucket bucket;
    bucket.startTime = 1730419200;
    bucket.endTime = 1730505600;
    bucket.results = {result};

    UsageBucket empty;
    empty.startTime = 1730505600;
    empty.endTime = 1730592000;

    UsagePage page;
    page.data = {bucket, empty};
    page.hasMore = true;
    page.nextPage = QStringLiteral("page_AAA");

    QCOMPARE(UsagePage::fromJson(page.toJson()), page);
    // The empty bucket is a bucket, not a missing one: a report has to keep its
    // gaps or a chart drawn from it silently closes them.
    QCOMPARE(UsagePage::fromJson(page.toJson()).data.at(1).size(), 0);
    QCOMPARE(UsagePage::fromJson(page.toJson()).data.at(1).endTime, qint64(1730592000));
}

void TestUsage::aCostResultRoundTripsThroughJson()
{
    CostResult result;
    result.setObject(QStringLiteral("organization.costs.result"));
    result.setAmount(CostAmount {0.06, QStringLiteral("usd")});
    result.setLineItem(QStringLiteral("gpt-4o, input"));
    result.setProjectId(QStringLiteral("proj_a"));

    const CostResult restored = CostResult::fromJson(result.toJson());
    QCOMPARE(restored, result);
    QCOMPARE(restored.amount().currency, QStringLiteral("usd"));
    QVERIFY(restored.amount().isValid());

    CostBucket bucket;
    bucket.startTime = 1730419200;
    bucket.endTime = 1730505600;
    bucket.results = {result};

    CostPage page;
    page.data = {bucket};
    QCOMPARE(CostPage::fromJson(page.toJson()), page);
    // No next_page on the last page, rather than a null one.
    QVERIFY(!page.toJson().contains(QStringLiteral("next_page")));

    // An amount the server did not report is not a free one.
    CostResult unreported;
    QVERIFY(!unreported.amount().isValid());
    QVERIFY(!unreported.toJson().contains(QStringLiteral("amount")));
}

void TestUsage::theQueryPutsEveryBucketParameterOnTheWire()
{
    // The acceptance criterion that matters most: a query the server does not
    // understand comes back as a valid report of the wrong thing, so what goes
    // on the wire is asserted rather than assumed.
    StubServer server(usagePage());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    UsageQuery query;
    query.startTime = 1730419200;
    query.endTime = 1730592000;
    query.bucketWidth = QStringLiteral("1d");
    query.groupBy = {QStringLiteral("model"), QStringLiteral("project_id")};
    query.limit = 7;
    query.page = QStringLiteral("page_AAA");
    query.projectIds = {QStringLiteral("proj_a"), QStringLiteral("proj_b")};
    query.userIds = {QStringLiteral("user_a")};
    query.apiKeyIds = {QStringLiteral("key_a")};
    query.models = {QStringLiteral("gpt-4o")};
    query.batch = true;

    QVERIFY(awaited(organization.usage(Organization::UsageKind::Completions, query)));

    const QByteArray line = server.requestLine();
    QVERIFY2(line.contains("start_time=1730419200"), line.constData());
    QVERIFY(line.contains("end_time=1730592000"));
    QVERIFY(line.contains("bucket_width=1d"));
    QVERIFY(line.contains("limit=7"));
    QVERIFY(line.contains("page=page_AAA"));
    QVERIFY(line.contains("batch=true"));
    // Array parameters repeat rather than joining with commas.
    QVERIFY(line.contains("group_by=model&group_by=project_id"));
    QVERIFY(line.contains("project_ids=proj_a&project_ids=proj_b"));
    QVERIFY(line.contains("user_ids=user_a"));
    QVERIFY(line.contains("api_key_ids=key_a"));
    QVERIFY(line.contains("models=gpt-4o"));
}

void TestUsage::theQueryOmitsWhatTheCallerDidNotSet()
{
    StubServer server(usagePage());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    UsageQuery query;
    query.startTime = 1730419200;
    QVERIFY(awaited(organization.usage(Organization::UsageKind::Embeddings, query)));

    const QByteArray line = server.requestLine();
    QVERIFY(line.contains("start_time=1730419200"));
    QVERIFY(!line.contains("end_time"));
    QVERIFY(!line.contains("bucket_width"));
    QVERIFY(!line.contains("group_by"));
    QVERIFY(!line.contains("limit"));
    QVERIFY(!line.contains("page="));
    QVERIFY(!line.contains("batch"));
    QVERIFY(!line.contains("_ids"));
    QVERIFY(!line.contains("models"));
}

void TestUsage::usageDecodesThePageAndReachesTheRightEndpoint()
{
    StubServer server(usagePage());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    UsageQuery query;
    query.startTime = 1730419200;
    const auto reply = awaited(organization.usage(Organization::UsageKind::Completions, query));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    QVERIFY(server.requestLine().startsWith("GET /v1/organization/usage/completions?"));
    QVERIFY(server.requestHeaders().contains("Authorization: Bearer sk-admin-test"));

    const UsagePage page = reply->usage();
    QCOMPARE(page.size(), 2);
    QVERIFY(page.hasMore);
    QCOMPARE(page.nextPage, QStringLiteral("page_AAA"));

    const UsageBucket &first = page.data.at(0);
    QCOMPARE(first.startTime, qint64(1730419200));
    QCOMPARE(first.size(), 1);
    const UsageResult &row = first.results.at(0);
    QCOMPARE(row.model(), QStringLiteral("gpt-4o"));
    QCOMPARE(row.inputTokens(), qint64(1200));
    QCOMPARE(row.inputCachedTokens(), qint64(800));
    QCOMPARE(row.numModelRequests(), qint64(7));
    QCOMPARE(row.batch(), std::optional<bool>(false));
    QVERIFY(row.projectId().isEmpty());

    QVERIFY(page.data.at(1).isEmpty());
}

void TestUsage::everyUsageKindHasItsOwnPath_data()
{
    QTest::addColumn<Organization::UsageKind>("kind");
    QTest::addColumn<QByteArray>("path");

    // Ten endpoints behind one method, so the mapping is what a typo would
    // break -- and a wrong path is a 404, not a wrong number.
    QTest::newRow("completions") << Organization::UsageKind::Completions
                                 << QByteArray("completions");
    QTest::newRow("embeddings") << Organization::UsageKind::Embeddings << QByteArray("embeddings");
    QTest::newRow("images") << Organization::UsageKind::Images << QByteArray("images");
    QTest::newRow("moderations") << Organization::UsageKind::Moderations
                                 << QByteArray("moderations");
    QTest::newRow("audio_speeches")
            << Organization::UsageKind::AudioSpeeches << QByteArray("audio_speeches");
    QTest::newRow("audio_transcriptions")
            << Organization::UsageKind::AudioTranscriptions << QByteArray("audio_transcriptions");
    QTest::newRow("vector_stores")
            << Organization::UsageKind::VectorStores << QByteArray("vector_stores");
    QTest::newRow("code_interpreter_sessions") << Organization::UsageKind::CodeInterpreterSessions
                                               << QByteArray("code_interpreter_sessions");
    QTest::newRow("file_search_calls")
            << Organization::UsageKind::FileSearchCalls << QByteArray("file_search_calls");
    QTest::newRow("web_search_calls")
            << Organization::UsageKind::WebSearchCalls << QByteArray("web_search_calls");
}

void TestUsage::everyUsageKindHasItsOwnPath()
{
    QFETCH(Organization::UsageKind, kind);
    QFETCH(QByteArray, path);

    StubServer server(usagePage());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    UsageQuery query;
    query.startTime = 1;
    QVERIFY(awaited(organization.usage(kind, query)));
    QCOMPARE(server.requestLine(), "GET /v1/organization/usage/" + path + "?start_time=1 HTTP/1.1");
}

void TestUsage::costsDecodeTheAmountAndItsCurrency()
{
    StubServer server(costsPage());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    UsageQuery query;
    query.startTime = 1730419200;
    query.groupBy = {QStringLiteral("line_item")};
    query.bucketWidth = QStringLiteral("1d");

    const auto reply = awaited(organization.costs(query));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    const QByteArray line = server.requestLine();
    QVERIFY2(line.startsWith("GET /v1/organization/costs?"), line.constData());
    QVERIFY(line.contains("start_time=1730419200"));
    QVERIFY(line.contains("group_by=line_item"));
    QVERIFY(line.contains("bucket_width=1d"));

    const CostPage page = reply->costs();
    QCOMPARE(page.size(), 1);
    QVERIFY(!page.hasMore);
    QVERIFY(page.nextPage.isEmpty());

    QCOMPARE(page.data.at(0).size(), 1);
    const CostResult &row = page.data.at(0).results.at(0);
    QCOMPARE(row.amount().value, 0.06);
    QCOMPARE(row.amount().currency, QStringLiteral("usd"));
    QCOMPARE(row.lineItem(), QStringLiteral("gpt-4o, input"));
    QCOMPARE(row.projectId(), QStringLiteral("proj_a"));
}

QTEST_MAIN(TestUsage)
#include "tst_usage.moc"
