// SPDX-License-Identifier: MIT
#include <QtOpenAi/Admin/Organization.h>
#include <QtOpenAi/Core/Project.h>
#include <QtOpenAi/Core/ProjectApiKey.h>
#include <QtOpenAi/Core/ProjectRateLimit.h>
#include <QtOpenAi/Core/ProjectServiceAccount.h>

#include <QtTest/QtTest>

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Admin;

namespace {

QByteArray projectPage()
{
    return R"({"object":"list","data":[
        {"id":"proj_active","object":"organization.project","name":"Production",
         "created_at":1711471533,"archived_at":null,"status":"active"}],
        "first_id":"proj_active","last_id":"proj_active","has_more":false})";
}

QByteArray apiKeyPage()
{
    return R"({"object":"list","data":[
        {"object":"organization.project.api_key","id":"key_person","name":"laptop",
         "redacted_value":"sk-abc...xyz","created_at":1711471533,"last_used_at":1711481533,
         "owner":{"type":"user","user":{"object":"organization.user","id":"user_ada",
                  "name":"Ada","email":"ada@example.com","role":"owner","added_at":17}}},
        {"object":"organization.project.api_key","id":"key_bot","name":"deploy",
         "redacted_value":"sk-def...uvw","created_at":1711471999,"last_used_at":null,
         "owner":{"type":"service_account","service_account":{
                  "object":"organization.project.service_account","id":"svc_1",
                  "name":"ci","role":"member","created_at":18}}}],
        "first_id":"key_person","last_id":"key_bot","has_more":true})";
}

} // namespace

// Coverage for the administration projects and their nested resources (#101,
// under #28). Offline: every request goes to the local stub server.
class TestProjects : public QObject
{
    Q_OBJECT
private slots:
    void aServiceAccountRoundTripsThroughJson();
    void anApiKeyRoundTripsThroughJson();
    void aRateLimitRoundTripsOnlyWhatWasSet();
    void listProjectsAndCreateOne();
    void archivingIsAPostThatChangesStatus();
    void nestedPathsAreComposedFromTheCollectionTable_data();
    void nestedPathsAreComposedFromTheCollectionTable();
    void listProjectApiKeysDecodesBothOwnerKinds();
    void creatingAServiceAccountReturnsTheOnlyCopyOfTheSecret();
    void modifyProjectRateLimitSendsOnlyTheLimitsThatWereSet();
    void addingAProjectUserPutsTheIdInTheBody();
};

void TestProjects::aServiceAccountRoundTripsThroughJson()
{
    ProjectServiceAccount account;
    account.setId(QStringLiteral("svc_1"));
    account.setObject(QStringLiteral("organization.project.service_account"));
    account.setName(QStringLiteral("ci"));
    account.setRole(QStringLiteral("member"));
    account.setCreatedAt(1711471533);

    QCOMPARE(ProjectServiceAccount::fromJson(account.toJson()), account);
    // A read-back account carries no key at all, which is not the same as a key
    // with an empty secret.
    QVERIFY(!account.apiKey().isValid());
    QVERIFY(!account.toJson().contains(QStringLiteral("api_key")));

    account.setApiKey(ServiceAccountApiKey {QStringLiteral("key_1"),
                                            QStringLiteral("organization.project.service_account"
                                                           ".api_key"),
                                            QStringLiteral("ci"), QStringLiteral("sk-secret"),
                                            1711471533});
    const ProjectServiceAccount restored = ProjectServiceAccount::fromJson(account.toJson());
    QCOMPARE(restored, account);
    QVERIFY(restored.apiKey().isValid());
    QCOMPARE(restored.apiKey().value, QStringLiteral("sk-secret"));
}

void TestProjects::anApiKeyRoundTripsThroughJson()
{
    OrganizationUser ada;
    ada.setId(QStringLiteral("user_ada"));
    ada.setName(QStringLiteral("Ada"));
    ada.setRole(QStringLiteral("owner"));

    ApiKeyOwner owner;
    owner.setType(QStringLiteral("user"));
    owner.setUser(ada);

    ProjectApiKey key;
    key.setId(QStringLiteral("key_person"));
    key.setObject(QStringLiteral("organization.project.api_key"));
    key.setName(QStringLiteral("laptop"));
    key.setRedactedValue(QStringLiteral("sk-abc...xyz"));
    key.setCreatedAt(1711471533);
    key.setLastUsedAt(1711481533);
    key.setOwner(owner);

    const ProjectApiKey restored = ProjectApiKey::fromJson(key.toJson());
    QCOMPARE(restored, key);
    QVERIFY(restored.owner().isUser());
    QVERIFY(!restored.owner().isServiceAccount());
    QCOMPARE(restored.owner().name(), QStringLiteral("Ada"));

    // A key that has never been used has no last-use time, and writing 1970
    // would sort it as the oldest instead of as the unused one.
    ProjectApiKey unused;
    unused.setId(QStringLiteral("key_new"));
    QCOMPARE(unused.lastUsedAt(), qint64(0));
    QVERIFY(!unused.toJson().contains(QStringLiteral("last_used_at")));
}

void TestProjects::aRateLimitRoundTripsOnlyWhatWasSet()
{
    ProjectRateLimit limit;
    QVERIFY(limit.isEmpty());

    limit.setId(QStringLiteral("rl_1"));
    limit.setObject(QStringLiteral("project.rate_limit"));
    limit.setModel(QStringLiteral("gpt-4o"));
    limit.setMaxRequestsPerMinute(600);
    limit.setMaxTokensPerMinute(150000);

    QVERIFY(!limit.isEmpty());
    const ProjectRateLimit restored = ProjectRateLimit::fromJson(limit.toJson());
    QCOMPARE(restored, limit);
    QCOMPARE(restored.maxRequestsPerMinute(), std::optional<qint64>(600));
    // The limits nobody mentioned stay unset rather than becoming zeros: a zero
    // here means the model is unusable in the project, which is a real setting
    // and not the same as "leave it alone".
    QVERIFY(!restored.maxImagesPerMinute().has_value());
    QVERIFY(!limit.toJson().contains(QStringLiteral("max_images_per_1_minute")));

    // And a limit the server really did report as 0 survives as 0.
    const ProjectRateLimit zeroed = ProjectRateLimit::fromJson(
            QJsonDocument::fromJson(R"({"id":"rl_2","max_requests_per_1_minute":0})").object());
    QCOMPARE(zeroed.maxRequestsPerMinute(), std::optional<qint64>(0));
    QCOMPARE(zeroed.toJson().value(QStringLiteral("max_requests_per_1_minute")).toInt(), 0);
}

void TestProjects::listProjectsAndCreateOne()
{
    StubServer list(projectPage());
    Organization organization(list.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto page = awaited(organization.listProjects());
    QVERIFY(page);
    QVERIFY2(page->isSuccess(), qPrintable(page->error().message()));
    QCOMPARE(page->projects().size(), 1);
    QVERIFY(list.requestLine().startsWith("GET /v1/organization/projects"));

    StubServer created(R"({"id":"proj_new","object":"organization.project","name":"Staging",
        "created_at":1711471533,"archived_at":null,"status":"active"})");
    Organization other(created.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(other.createProject(QStringLiteral("Staging")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(created.requestLine(), "POST /v1/organization/projects HTTP/1.1");
    QCOMPARE(created.requestBody(), R"({"name":"Staging"})");
    QCOMPARE(reply->project().name(), QStringLiteral("Staging"));
    QVERIFY(!reply->project().isArchived());
}

void TestProjects::archivingIsAPostThatChangesStatus()
{
    // It reads like a delete and is not one: the project stays, with its status
    // changed, because usage and cost records point at it.
    StubServer server(R"({"id":"proj_old","object":"organization.project","name":"Retired",
        "created_at":1611471533,"archived_at":1711471533,"status":"archived"})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(organization.archiveProject(QStringLiteral("proj_old")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    QCOMPARE(server.requestLine(), "POST /v1/organization/projects/proj_old/archive HTTP/1.1");
    QVERIFY(server.requestBody().isEmpty());
    QVERIFY(reply->project().isArchived());
    QCOMPARE(reply->project().archivedAt(), qint64(1711471533));
}

void TestProjects::nestedPathsAreComposedFromTheCollectionTable_data()
{
    QTest::addColumn<int>("endpoint");
    QTest::addColumn<QByteArray>("line");

    // Every nested collection repeats the same shape under a project id, so what
    // a typo would break is the composition -- and a wrong path is a 404 rather
    // than a wrong answer.
    QTest::newRow("users") << 0
                           << QByteArray("GET /v1/organization/projects/proj_1/users HTTP/1.1");
    QTest::newRow("user") << 1
                          << QByteArray("GET /v1/organization/projects/proj_1/users/user_1 "
                                        "HTTP/1.1");
    QTest::newRow("service_accounts")
            << 2 << QByteArray("GET /v1/organization/projects/proj_1/service_accounts HTTP/1.1");
    QTest::newRow("service_account")
            << 3
            << QByteArray("GET /v1/organization/projects/proj_1/service_accounts/svc_1 HTTP/1.1");
    QTest::newRow("api_keys") << 4
                              << QByteArray("GET /v1/organization/projects/proj_1/api_keys "
                                            "HTTP/1.1");
    QTest::newRow("api_key") << 5
                             << QByteArray("GET /v1/organization/projects/proj_1/api_keys/key_1 "
                                           "HTTP/1.1");
    QTest::newRow("rate_limits")
            << 6 << QByteArray("GET /v1/organization/projects/proj_1/rate_limits HTTP/1.1");
    QTest::newRow("project") << 7 << QByteArray("GET /v1/organization/projects/proj_1 HTTP/1.1");
}

void TestProjects::nestedPathsAreComposedFromTheCollectionTable()
{
    QFETCH(int, endpoint);
    QFETCH(QByteArray, line);

    StubServer server(QByteArray("{}"));
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));
    const QString project = QStringLiteral("proj_1");

    switch (endpoint) {
    case 0:
        QVERIFY(awaited(organization.listProjectUsers(project)));
        break;
    case 1:
        QVERIFY(awaited(organization.getProjectUser(project, QStringLiteral("user_1"))));
        break;
    case 2:
        QVERIFY(awaited(organization.listProjectServiceAccounts(project)));
        break;
    case 3:
        QVERIFY(awaited(organization.getProjectServiceAccount(project, QStringLiteral("svc_1"))));
        break;
    case 4:
        QVERIFY(awaited(organization.listProjectApiKeys(project)));
        break;
    case 5:
        QVERIFY(awaited(organization.getProjectApiKey(project, QStringLiteral("key_1"))));
        break;
    case 6:
        QVERIFY(awaited(organization.listProjectRateLimits(project)));
        break;
    default:
        QVERIFY(awaited(organization.getProject(project)));
        break;
    }

    QCOMPARE(server.requestLine(), line);
}

void TestProjects::listProjectApiKeysDecodesBothOwnerKinds()
{
    StubServer server(apiKeyPage());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(organization.listProjectApiKeys(QStringLiteral("proj_1")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QVERIFY(server.requestHeaders().contains("Authorization: Bearer sk-admin-test"));

    const ProjectApiKeyList page = reply->apiKeys();
    QCOMPARE(page.size(), 2);
    QVERIFY(page.hasMore);

    // Which kind of principal holds a key is the question an audit asks first,
    // so the tagged union is kept rather than flattened to one name.
    const ProjectApiKey &person = page.data.at(0);
    QVERIFY(person.owner().isUser());
    QCOMPARE(person.owner().user().email(), QStringLiteral("ada@example.com"));
    QCOMPARE(person.redactedValue(), QStringLiteral("sk-abc...xyz"));
    QCOMPARE(person.lastUsedAt(), qint64(1711481533));

    const ProjectApiKey &bot = page.data.at(1);
    QVERIFY(bot.owner().isServiceAccount());
    QCOMPARE(bot.owner().serviceAccount().name(), QStringLiteral("ci"));
    QCOMPARE(bot.owner().name(), QStringLiteral("ci"));
    // Never used, and reported as such rather than as 1970.
    QCOMPARE(bot.lastUsedAt(), qint64(0));
}

void TestProjects::creatingAServiceAccountReturnsTheOnlyCopyOfTheSecret()
{
    StubServer server(R"({"object":"organization.project.service_account","id":"svc_1",
        "name":"ci","role":"member","created_at":1711471533,
        "api_key":{"object":"organization.project.service_account.api_key","value":"sk-live-secret",
                   "name":"ci","created_at":1711471533,"id":"key_1"}})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(organization.createProjectServiceAccount(QStringLiteral("proj_1"),
                                                                        QStringLiteral("ci")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    QCOMPARE(server.requestLine(),
             "POST /v1/organization/projects/proj_1/service_accounts HTTP/1.1");
    QCOMPARE(server.requestBody(), R"({"name":"ci"})");

    // The one response that carries the secret; every later read redacts it.
    const ProjectServiceAccount account = reply->serviceAccount();
    QVERIFY(account.apiKey().isValid());
    QCOMPARE(account.apiKey().value, QStringLiteral("sk-live-secret"));
    QCOMPARE(account.role(), QStringLiteral("member"));
}

void TestProjects::modifyProjectRateLimitSendsOnlyTheLimitsThatWereSet()
{
    StubServer server(R"({"object":"project.rate_limit","id":"rl_1","model":"gpt-4o",
        "max_requests_per_1_minute":600,"max_tokens_per_1_minute":150000})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    // Read back from the server first, so the request carries an id, an object
    // and a model it must not send back as changes.
    ProjectRateLimit limits;
    limits.setId(QStringLiteral("rl_1"));
    limits.setObject(QStringLiteral("project.rate_limit"));
    limits.setModel(QStringLiteral("gpt-4o"));
    limits.setMaxRequestsPerMinute(600);

    const auto reply = awaited(organization.modifyProjectRateLimit(QStringLiteral("proj_1"),
                                                                   QStringLiteral("rl_1"), limits));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    QCOMPARE(server.requestLine(),
             "POST /v1/organization/projects/proj_1/rate_limits/rl_1 HTTP/1.1");
    // Exactly the one limit that was set: the untouched ones are absent rather
    // than zero, and the identifying fields are not resent as changes.
    QCOMPARE(server.requestBody(), R"({"max_requests_per_1_minute":600})");

    QCOMPARE(reply->rateLimit().maxTokensPerMinute(), std::optional<qint64>(150000));
}

void TestProjects::addingAProjectUserPutsTheIdInTheBody()
{
    StubServer server(R"({"object":"organization.project.user","id":"user_ada","name":"Ada",
        "email":"ada@example.com","role":"member","added_at":1711471533})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(organization.createProjectUser(
            QStringLiteral("proj_1"), QStringLiteral("user_ada"), QStringLiteral("member")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    // The id goes in the body, not the path: this adds an existing organization
    // member to the project rather than creating a person.
    QCOMPARE(server.requestLine(), "POST /v1/organization/projects/proj_1/users HTTP/1.1");
    QCOMPARE(server.requestBody(), R"({"role":"member","user_id":"user_ada"})");

    // The project role, not the organization one -- the same value type either
    // way, because the six fields are the same.
    QCOMPARE(reply->user().role(), QStringLiteral("member"));
    QVERIFY(!reply->user().isOwner());
}

QTEST_MAIN(TestProjects)
#include "tst_projects.moc"
