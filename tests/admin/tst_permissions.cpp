// SPDX-License-Identifier: MIT
#include <QtOpenAi/Admin/Organization.h>
#include <QtOpenAi/Core/ProjectPermissions.h>

#include <QtTest/QtTest>

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Admin;

// Coverage for the project model and hosted-tool permissions (#110, split out of
// #101, under #28). Offline: every request goes to the local stub server.
class TestPermissions : public QObject
{
    Q_OBJECT
private slots:
    void modelPermissionsRoundTripThroughJson();
    void theModeDecidesWhatTheIdsMean_data();
    void theModeDecidesWhatTheIdsMean();
    void anUnreadableModeAnswersNothingRatherThanYes();
    void hostedToolPermissionsRoundTripThroughJson();
    void anUnknownHostedToolSurvivesTheRoundTrip();
    void getAndSetModelPermissions();
    void deletingTheModelPolicyIsAcknowledged();
    void listHostedToolPermissions();
    void aHostedToolUpdateSendsOnlyTheToolsThatWereSet();
};

void TestPermissions::modelPermissionsRoundTripThroughJson()
{
    ProjectModelPermissions permissions;
    permissions.setObject(QStringLiteral("project.model_permissions"));
    permissions.setMode(QStringLiteral("allow_list"));
    permissions.setModelIds({QStringLiteral("gpt-4.1"), QStringLiteral("o3")});

    QCOMPARE(ProjectModelPermissions::fromJson(permissions.toJson()), permissions);
    QVERIFY(permissions.isAllowList());
    QVERIFY(!permissions.isDenyList());
    QVERIFY(!permissions.isDeleted());
}

void TestPermissions::theModeDecidesWhatTheIdsMean_data()
{
    QTest::addColumn<QString>("mode");
    QTest::addColumn<bool>("listedIsAllowed");

    // The same id, the same list, opposite answers. This is the whole reason
    // allowsModel() exists: reading modelIds() without the mode fails *open*
    // on a deny list, handing out every model it was meant to withhold.
    QTest::newRow("allow_list") << QStringLiteral("allow_list") << true;
    QTest::newRow("deny_list") << QStringLiteral("deny_list") << false;
}

void TestPermissions::theModeDecidesWhatTheIdsMean()
{
    QFETCH(QString, mode);
    QFETCH(bool, listedIsAllowed);

    ProjectModelPermissions permissions;
    permissions.setMode(mode);
    permissions.setModelIds({QStringLiteral("gpt-4.1")});

    QCOMPARE(permissions.allowsModel(QStringLiteral("gpt-4.1")),
             std::optional<bool>(listedIsAllowed));
    // And a model the policy never names is the opposite answer again.
    QCOMPARE(permissions.allowsModel(QStringLiteral("o3")), std::optional<bool>(!listedIsAllowed));
}

void TestPermissions::anUnreadableModeAnswersNothingRatherThanYes()
{
    // A mode added after this build. Answering "allowed" would fail open on the
    // one question this class exists to answer, so it answers nothing at all
    // and the caller has to notice.
    ProjectModelPermissions permissions;
    permissions.setMode(QStringLiteral("quarantine_list"));
    permissions.setModelIds({QStringLiteral("gpt-4.1")});

    QVERIFY(!permissions.allowsModel(QStringLiteral("gpt-4.1")).has_value());
    QVERIFY(!permissions.allowsModel(QStringLiteral("o3")).has_value());
    QVERIFY(!permissions.isAllowList());
    QVERIFY(!permissions.isDenyList());
    // The unknown mode still survives the round trip rather than decaying to
    // one of the two this build knows.
    QCOMPARE(ProjectModelPermissions::fromJson(permissions.toJson()).mode(),
             QStringLiteral("quarantine_list"));
}

void TestPermissions::hostedToolPermissionsRoundTripThroughJson()
{
    ProjectHostedToolPermissions permissions;
    QVERIFY(permissions.isEmpty());

    permissions.setFileSearch(true);
    permissions.setWebSearch(false);
    permissions.setImageGeneration(true);
    permissions.setMcp(false);
    permissions.setCodeInterpreter(true);

    QVERIFY(!permissions.isEmpty());
    // Setting every typed accessor lands exactly the names knownTools() lists:
    // the two drift apart the moment one gains a tool the other does not.
    QCOMPARE(permissions.permissions().keys(), ProjectHostedToolPermissions::knownTools());
    QCOMPARE(ProjectHostedToolPermissions::fromJson(permissions.toJson()), permissions);
    QCOMPARE(permissions.fileSearch(), std::optional<bool>(true));
    QCOMPARE(permissions.webSearch(), std::optional<bool>(false));
    QCOMPARE(permissions.mcp(), std::optional<bool>(false));

    // Each switch is its own object on the wire, not a bare boolean.
    const QJsonObject json = permissions.toJson();
    QCOMPARE(json.value(QStringLiteral("web_search"))
                     .toObject()
                     .value(QStringLiteral("enabled"))
                     .toBool(),
             false);

    // A tool nobody mentioned is unset rather than false -- on an update that is
    // the difference between "leave it alone" and "switch it off".
    ProjectHostedToolPermissions partial;
    partial.setWebSearch(true);
    QVERIFY(!partial.fileSearch().has_value());
    QVERIFY(!partial.toJson().contains(QStringLiteral("file_search")));
}

void TestPermissions::anUnknownHostedToolSurvivesTheRoundTrip()
{
    // A tool added after this build has no typed accessor, and is still carried
    // rather than dropped -- the same bargain UsageResult makes for a counter it
    // has never heard of.
    const ProjectHostedToolPermissions permissions = ProjectHostedToolPermissions::fromJson(
            QJsonDocument::fromJson(R"({"web_search":{"enabled":true},
                "computer_use":{"enabled":false}})")
                    .object());

    QCOMPARE(permissions.webSearch(), std::optional<bool>(true));
    QCOMPARE(permissions.permission(QStringLiteral("computer_use")), std::optional<bool>(false));
    QCOMPARE(permissions.permissions().size(), 2);
    QCOMPARE(ProjectHostedToolPermissions::fromJson(permissions.toJson()), permissions);

    // ...and it is findable as one, so a caller enumerating the record can tell
    // the tools this build names from the ones it merely carries.
    QVERIFY(ProjectHostedToolPermissions::isKnownTool(QStringLiteral("web_search")));
    QVERIFY(!ProjectHostedToolPermissions::isKnownTool(QStringLiteral("computer_use")));

    // A sibling key that is not a `{"enabled": ...}` object is not a tool, and
    // does not decode as one that happens to be switched off.
    const ProjectHostedToolPermissions noisy = ProjectHostedToolPermissions::fromJson(
            QJsonDocument::fromJson(R"({"object":"project.hosted_tool_permissions",
                "web_search":{"enabled":true}})")
                    .object());
    QCOMPARE(noisy.permissions().size(), 1);
    QVERIFY(!noisy.permission(QStringLiteral("object")).has_value());
}

void TestPermissions::getAndSetModelPermissions()
{
    StubServer read(R"({"object":"project.model_permissions","mode":"allow_list",
        "model_ids":["gpt-4.1","o3"]})");
    Organization organization(read.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(organization.getProjectModelPermissions(QStringLiteral("proj_1")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(read.requestLine(), "GET /v1/organization/projects/proj_1/model_permissions HTTP/1.1");
    QVERIFY(read.requestHeaders().contains("Authorization: Bearer sk-admin-test"));
    QCOMPARE(reply->permissions().allowsModel(QStringLiteral("o3")), std::optional<bool>(true));

    StubServer written(R"({"object":"project.model_permissions","mode":"deny_list",
        "model_ids":["gpt-4.1"]})");
    Organization other(written.baseUrl(), QStringLiteral("sk-admin-test"));

    // Read back from the server first, so the value carries an object the
    // request must not send back as a change.
    ProjectModelPermissions policy = reply->permissions();
    policy.setMode(QStringLiteral("deny_list"));
    policy.setModelIds({QStringLiteral("gpt-4.1")});

    const auto updated
            = awaited(other.setProjectModelPermissions(QStringLiteral("proj_1"), policy));
    QVERIFY(updated);
    QVERIFY2(updated->isSuccess(), qPrintable(updated->error().message()));
    QCOMPARE(written.requestLine(),
             "POST /v1/organization/projects/proj_1/model_permissions HTTP/1.1");
    // The policy is replaced whole -- both fields every time -- and the
    // identifying object is not resent as a change.
    QCOMPARE(written.requestBody(), R"({"mode":"deny_list","model_ids":["gpt-4.1"]})");

    QVERIFY(updated->permissions().isDenyList());
    QCOMPARE(updated->permissions().allowsModel(QStringLiteral("gpt-4.1")),
             std::optional<bool>(false));
}

void TestPermissions::deletingTheModelPolicyIsAcknowledged()
{
    StubServer server(R"({"object":"project.model_permissions.deleted","deleted":true})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply
            = awaited(organization.deleteProjectModelPermissions(QStringLiteral("proj_1")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(server.requestLine(),
             "DELETE /v1/organization/projects/proj_1/model_permissions HTTP/1.1");

    // Deleting the policy is how a project falls back to the organization's
    // default, so the acknowledgement carries no policy at all.
    QVERIFY(reply->permissions().isDeleted());
    QVERIFY(reply->permissions().modelIds().isEmpty());
    QVERIFY(reply->permissions().mode().isEmpty());
    // And with no mode there is nothing to answer with.
    QVERIFY(!reply->permissions().allowsModel(QStringLiteral("gpt-4.1")).has_value());
}

void TestPermissions::listHostedToolPermissions()
{
    StubServer server(R"({"file_search":{"enabled":true},"web_search":{"enabled":false},
        "image_generation":{"enabled":true},"mcp":{"enabled":false},
        "code_interpreter":{"enabled":true}})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply
            = awaited(organization.getProjectHostedToolPermissions(QStringLiteral("proj_1")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(server.requestLine(),
             "GET /v1/organization/projects/proj_1/hosted_tool_permissions HTTP/1.1");

    const ProjectHostedToolPermissions permissions = reply->permissions();
    QCOMPARE(permissions.permissions().size(), 5);
    QCOMPARE(permissions.fileSearch(), std::optional<bool>(true));
    QCOMPARE(permissions.webSearch(), std::optional<bool>(false));
    QCOMPARE(permissions.imageGeneration(), std::optional<bool>(true));
    QCOMPARE(permissions.mcp(), std::optional<bool>(false));
    QCOMPARE(permissions.codeInterpreter(), std::optional<bool>(true));
}

void TestPermissions::aHostedToolUpdateSendsOnlyTheToolsThatWereSet()
{
    StubServer server(R"({"file_search":{"enabled":true},"web_search":{"enabled":false},
        "image_generation":{"enabled":true},"mcp":{"enabled":true},
        "code_interpreter":{"enabled":true}})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    ProjectHostedToolPermissions permissions;
    permissions.setWebSearch(false);

    const auto reply = awaited(
            organization.setProjectHostedToolPermissions(QStringLiteral("proj_1"), permissions));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(server.requestLine(),
             "POST /v1/organization/projects/proj_1/hosted_tool_permissions HTTP/1.1");
    // Exactly the one tool that was set: the untouched four are absent rather
    // than sent as false, which would have switched off everything the caller
    // did not mention.
    QCOMPARE(server.requestBody(), R"({"web_search":{"enabled":false}})");

    // The reply is the whole record, whatever the request touched.
    QCOMPARE(reply->permissions().permissions().size(), 5);
}

QTEST_MAIN(TestPermissions)
#include "tst_permissions.moc"
