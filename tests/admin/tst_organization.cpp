// SPDX-License-Identifier: MIT
#include <QtOpenAi/Admin/Organization.h>
#include <QtOpenAi/Client/Interceptor.h>
#include <QtOpenAi/Core/Project.h>

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
         "created_at":1711471533,"archived_at":null,"status":"active"},
        {"id":"proj_old","object":"organization.project","name":"Retired",
         "created_at":1611471533,"archived_at":1711471533,"status":"archived"}],
        "first_id":"proj_active","last_id":"proj_old","has_more":true})";
}

// Records the request it is given, so a test can prove the administration
// surface really runs through Client's interceptor chain rather than a request
// path of its own.
class RecordingInterceptor : public QtOpenAi::Client::Interceptor
{
public:
    std::optional<QtOpenAi::Client::InterceptedResponse>
    beforeRequest(QtOpenAi::Client::InterceptedRequest &request) override
    {
        ++seen;
        method = request.method;
        url = request.url();
        authorization = request.request.rawHeader("Authorization");
        request.request.setRawHeader("X-Trace", "from-the-chain");
        return std::nullopt;
    }

    void afterResponse(const QtOpenAi::Client::InterceptedResponse &response) override
    {
        ++answered;
        status = response.httpStatus;
    }

    int seen = 0;
    int answered = 0;
    int status = 0;
    QByteArray method;
    QUrl url;
    QByteArray authorization;
};

} // namespace

// Coverage for the QtOpenAi::Admin module skeleton (#98, under #28). Offline:
// every request goes to the local stub server, and no admin key leaves it.
class TestOrganization : public QObject
{
    Q_OBJECT
private slots:
    void aProjectRoundTripsThroughJson();
    void anActiveProjectHasNoArchivalStamp();
    void listProjectsSendsTheAdminKeyAndDecodesThePage();
    void listProjectsPassesPaginationAndTheArchivedFlag();
    void theRequestGoesThroughTheClientInterceptorChain();
    void configurationReachesTheRequest();
};

void TestOrganization::aProjectRoundTripsThroughJson()
{
    Project project;
    project.setId(QStringLiteral("proj_abc"));
    project.setObject(QStringLiteral("organization.project"));
    project.setName(QStringLiteral("Production"));
    project.setCreatedAt(1711471533);
    project.setArchivedAt(1711475000);
    project.setStatus(QStringLiteral("archived"));

    const Project restored = Project::fromJson(project.toJson());
    QCOMPARE(restored, project);
    QVERIFY(restored.isArchived());

    ProjectList page;
    page.data = {project};
    page.firstId = QStringLiteral("proj_abc");
    page.lastId = QStringLiteral("proj_abc");
    page.hasMore = true;
    QCOMPARE(ProjectList::fromJson(page.toJson()), page);
}

void TestOrganization::anActiveProjectHasNoArchivalStamp()
{
    // The API sends `"archived_at": null` for a live project, and a status this
    // build has never heard of has to survive rather than decay to a guess.
    const QJsonObject json = QJsonDocument::fromJson(R"({"id":"proj_x",
        "object":"organization.project","name":"New","created_at":17,
        "archived_at":null,"status":"pending_deletion"})")
                                     .object();

    const Project project = Project::fromJson(json);
    QCOMPARE(project.archivedAt(), qint64(0));
    QVERIFY(!project.isArchived());
    QCOMPARE(project.status(), QStringLiteral("pending_deletion"));
    // And it is still the status on the way back out.
    QCOMPARE(Project::fromJson(project.toJson()).status(), QStringLiteral("pending_deletion"));
    // A null archived_at is written as an absent key, not as a zero.
    QVERIFY(!project.toJson().contains(QStringLiteral("archived_at")));
}

void TestOrganization::listProjectsSendsTheAdminKeyAndDecodesThePage()
{
    // The acceptance criterion: one representative endpoint round-trips, with
    // the admin key on the request and the page decoded from the response.
    StubServer server(projectPage());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(organization.listProjects());
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    QVERIFY(server.requestLine().startsWith("GET /v1/organization/projects"));
    QVERIFY(server.requestHeaders().contains("Authorization: Bearer sk-admin-test"));

    const ProjectList page = reply->projects();
    QCOMPARE(page.size(), 2);
    QVERIFY(page.hasMore);
    QCOMPARE(page.firstId, QStringLiteral("proj_active"));
    QCOMPARE(page.data.at(0).name(), QStringLiteral("Production"));
    QVERIFY(!page.data.at(0).isArchived());
    QCOMPARE(page.data.at(1).status(), QStringLiteral("archived"));
    QCOMPARE(page.data.at(1).archivedAt(), qint64(1711471533));
}

void TestOrganization::listProjectsPassesPaginationAndTheArchivedFlag()
{
    StubServer server(projectPage());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    QtOpenAi::Client::ListParams params;
    params.after = QStringLiteral("proj_active");
    params.limit = 20;

    QVERIFY(awaited(organization.listProjects(params, true)));
    const QByteArray line = server.requestLine();
    QVERIFY(line.contains("after=proj_active"));
    QVERIFY(line.contains("limit=20"));
    QVERIFY(line.contains("include_archived=true"));
}

void TestOrganization::theRequestGoesThroughTheClientInterceptorChain()
{
    // The whole point of the request seam: the administration surface is not a
    // second request path, so everything installed on a client -- logging with
    // the credential redacted, a trace header, a cache -- covers it too.
    StubServer server(projectPage());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    RecordingInterceptor interceptor;
    organization.addInterceptor(&interceptor);

    QVERIFY(awaited(organization.listProjects()));

    QCOMPARE(interceptor.seen, 1);
    QCOMPARE(interceptor.method, QByteArray("GET"));
    QCOMPARE(interceptor.url.path(), QStringLiteral("/v1/organization/projects"));
    QCOMPARE(interceptor.authorization, QByteArray("Bearer sk-admin-test"));
    // What the chain changed is what went on the wire.
    QVERIFY(server.requestHeaders().contains("X-Trace: from-the-chain"));

    QCOMPARE(interceptor.answered, 1);
    QCOMPARE(interceptor.status, 200);
}

void TestOrganization::configurationReachesTheRequest()
{
    StubServer server(projectPage());
    Organization organization;
    QSignalSpy replies(&organization, &Organization::replyCreated);
    QSignalSpy keys(&organization, &Organization::adminKeyChanged);

    organization.setBaseUrl(server.baseUrl());
    organization.setAdminKey(QStringLiteral("sk-admin-late"));

    QVERIFY(awaited(organization.listProjects()));
    QVERIFY(server.requestHeaders().contains("Authorization: Bearer sk-admin-late"));
    // The reply is announced, so a MetricsCollector can watch this surface too.
    QCOMPARE(replies.count(), 1);
    QCOMPARE(keys.count(), 1);

    // Setting the same key again is not a change.
    organization.setAdminKey(QStringLiteral("sk-admin-late"));
    QCOMPARE(keys.count(), 1);
}

QTEST_MAIN(TestOrganization)
#include "tst_organization.moc"
