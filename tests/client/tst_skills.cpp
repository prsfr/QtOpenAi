// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

// Offline stub-server coverage for the Skills endpoints (#27): the multipart
// upload of a bundle, the version family below it, the default-version pointer
// and the two zip downloads.
class TestSkillsClient : public QObject
{
    Q_OBJECT
private slots:
    void createUploadsBundleAsMultipart();
    void createSendsOneFilePartPerBundleFile();
    void listSendsPaginationQuery();
    void getParsesSkill();
    void setDefaultVersionPostsPointer();
    void deleteIssuesDeleteVerb();
    void createVersionSendsDefaultFlag();
    void listVersionsParsesPage();
    void getVersionUsesVersionPath();
    void deleteVersionIssuesDeleteVerb();
    void downloadContentReturnsZipBytes();
    void downloadVersionContentUsesVersionPath();
};

void TestSkillsClient::createUploadsBundleAsMultipart()
{
    StubServer server(QByteArray(R"({"id":"skill_123","object":"skill","name":"pdf-report",)"
                                 R"("default_version":"1","latest_version":"1"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const CreateSkillRequest request(QStringLiteral("pdf-report.zip"), QByteArray("PK\x03\x04"));
    const auto reply = awaited(client.createSkill(request));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/skills "));
    QVERIFY(server.requestHeaders().toLower().contains("content-type: multipart/form-data"));
    QVERIFY(server.requestBody().contains("name=\"files\"; filename=\"pdf-report.zip\""));
    QCOMPARE(reply->skill().id(), QStringLiteral("skill_123"));
    QCOMPARE(reply->skill().latestVersion(), QStringLiteral("1"));
}

void TestSkillsClient::createSendsOneFilePartPerBundleFile()
{
    // A directory upload is one `files` part per file, each keeping its path
    // relative to the skill root.
    StubServer server(QByteArray(R"({"id":"skill_123"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateSkillRequest request;
    request.addFile(QStringLiteral("SKILL.md"), QByteArray("# pdf-report"));
    request.addFile(QStringLiteral("scripts/build.py"), QByteArray("print(1)"));

    const auto reply = awaited(client.createSkill(request));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    const QByteArray body = server.requestBody();
    QVERIFY(body.contains("name=\"files\"; filename=\"SKILL.md\""));
    QVERIFY(body.contains("name=\"files\"; filename=\"scripts/build.py\""));
    QVERIFY(body.contains("print(1)"));
}

void TestSkillsClient::listSendsPaginationQuery()
{
    StubServer server(QByteArray(R"({"object":"list","data":[{"id":"skill_1"},{"id":"skill_2"}],)"
                                 R"("first_id":"skill_1","last_id":"skill_2","has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.limit = 2;
    params.order = QStringLiteral("asc");
    const auto reply = awaited(client.listSkills(params));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/skills?"));
    QVERIFY(server.requestLine().contains("limit=2"));
    QVERIFY(server.requestLine().contains("order=asc"));
    QCOMPARE(reply->list().size(), 2);
    QCOMPARE(reply->list().lastId, QStringLiteral("skill_2"));
}

void TestSkillsClient::getParsesSkill()
{
    StubServer server(QByteArray(R"({"id":"skill_123","object":"skill","created_at":1716028800,)"
                                 R"("description":"Builds a PDF report","default_version":"2",)"
                                 R"("latest_version":"3"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.getSkill(QStringLiteral("skill_123")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/skills/skill_123 "));
    QCOMPARE(reply->skill().createdAt(), Q_INT64_C(1716028800));
    QCOMPARE(reply->skill().defaultVersion(), QStringLiteral("2"));
    QCOMPARE(reply->skill().latestVersion(), QStringLiteral("3"));
}

void TestSkillsClient::setDefaultVersionPostsPointer()
{
    StubServer server(QByteArray(R"({"id":"skill_123","default_version":"3"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(
            client.setDefaultSkillVersion(QStringLiteral("skill_123"), QStringLiteral("3")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/skills/skill_123 "));
    // Promoting a version is the only thing this endpoint does, so the body
    // carries the pointer and nothing else.
    QCOMPARE(server.requestBody(), QByteArray(R"({"default_version":"3"})"));
    QCOMPARE(reply->skill().defaultVersion(), QStringLiteral("3"));
}

void TestSkillsClient::deleteIssuesDeleteVerb()
{
    StubServer server(QByteArray(R"({"id":"skill_123","object":"skill.deleted","deleted":true})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.deleteSkill(QStringLiteral("skill_123")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("DELETE /v1/skills/skill_123 "));
    QCOMPARE(reply->skill().object(), QStringLiteral("skill.deleted"));
}

void TestSkillsClient::createVersionSendsDefaultFlag()
{
    StubServer server(QByteArray(R"({"object":"skill.version","id":"skillver_456",)"
                                 R"("skill_id":"skill_123","version":"3"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateSkillRequest request(QStringLiteral("bundle.zip"), QByteArray("PK\x03\x04"));
    request.setMakeDefault(true);

    const auto reply = awaited(client.createSkillVersion(QStringLiteral("skill_123"), request));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/skills/skill_123/versions "));
    const QByteArray body = server.requestBody();
    QVERIFY(body.contains("name=\"default\""));
    QVERIFY(body.contains("true"));
    QVERIFY(body.contains("name=\"files\"; filename=\"bundle.zip\""));
    QCOMPARE(reply->version().skillId(), QStringLiteral("skill_123"));
    QCOMPARE(reply->version().version(), QStringLiteral("3"));
}

void TestSkillsClient::listVersionsParsesPage()
{
    StubServer server(QByteArray(R"({"object":"list","data":[{"id":"skillver_1","version":"1"},)"
                                 R"({"id":"skillver_2","version":"2"}],"has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.listSkillVersions(QStringLiteral("skill_123")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/skills/skill_123/versions "));
    QCOMPARE(reply->list().size(), 2);
    QCOMPARE(reply->list().data.at(1).version(), QStringLiteral("2"));
}

void TestSkillsClient::getVersionUsesVersionPath()
{
    StubServer server(QByteArray(R"({"id":"skillver_2","version":"2","name":"pdf-report"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply
            = awaited(client.getSkillVersion(QStringLiteral("skill_123"), QStringLiteral("2")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/skills/skill_123/versions/2 "));
    QCOMPARE(reply->version().name(), QStringLiteral("pdf-report"));
}

void TestSkillsClient::deleteVersionIssuesDeleteVerb()
{
    StubServer server(QByteArray(R"({"id":"skillver_2","object":"skill.version.deleted",)"
                                 R"("version":"2","deleted":true})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply
            = awaited(client.deleteSkillVersion(QStringLiteral("skill_123"), QStringLiteral("2")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("DELETE /v1/skills/skill_123/versions/2 "));
    QCOMPARE(reply->version().object(), QStringLiteral("skill.version.deleted"));
}

void TestSkillsClient::downloadContentReturnsZipBytes()
{
    // The bundle comes back as a zip, so it is served verbatim rather than
    // decoded.
    StubServer server(QByteArray("PK\x03\x04bundle", 10), QByteArray("application/zip"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.downloadSkillContent(QStringLiteral("skill_123")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/skills/skill_123/content "));
    QCOMPARE(reply->data(), QByteArray("PK\x03\x04bundle", 10));
    QCOMPARE(reply->contentType(), QByteArray("application/zip"));
}

void TestSkillsClient::downloadVersionContentUsesVersionPath()
{
    StubServer server(QByteArray("PK\x03\x04v2", 6), QByteArray("application/zip"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(
            client.downloadSkillVersionContent(QStringLiteral("skill_123"), QStringLiteral("2")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/skills/skill_123/versions/2/content "));
    QCOMPARE(reply->data(), QByteArray("PK\x03\x04v2", 6));
}

QTEST_MAIN(TestSkillsClient)
#include "tst_skills.moc"
