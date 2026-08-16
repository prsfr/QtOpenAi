// SPDX-License-Identifier: MIT
#include <QtOpenAi/Admin/Organization.h>
#include <QtOpenAi/Core/DataRetention.h>
#include <QtOpenAi/Core/SpendAlert.h>

#include <QtTest/QtTest>

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Admin;

namespace {

// $1,000.00, which the API spells as 100000 cents. The two numbers are written
// next to each other here on purpose: this file exists partly to keep the
// factor of a hundred from drifting.
constexpr qint64 kThousandDollars = 100000;

QByteArray alertJson(const char *object = "organization.spend_alert")
{
    return QByteArray(R"({"id":"alert_abc123","object":")") + object
           + R"(","threshold_amount":100000,"currency":"USD","interval":"month",
        "notification_channel":{"type":"email","recipients":["finance@example.com"],
                                "subject_prefix":"OpenAI spend alert"}})";
}

QByteArray alertPage()
{
    return R"({"object":"list","data":[
        {"id":"alert_abc123","object":"organization.spend_alert","threshold_amount":100000,
         "currency":"USD","interval":"month",
         "notification_channel":{"type":"email","recipients":["finance@example.com"]}},
        {"id":"alert_def456","object":"organization.spend_alert","threshold_amount":0,
         "currency":"USD","interval":"month",
         "notification_channel":{"type":"email","recipients":["ops@example.com","cfo@example.com"],
                                 "subject_prefix":"[budget]"}}],
        "first_id":"alert_abc123","last_id":"alert_def456","has_more":false})";
}

SpendAlert sampleAlert()
{
    SpendAlertNotificationChannel channel;
    channel.setRecipients({QStringLiteral("finance@example.com")});
    channel.setSubjectPrefix(QStringLiteral("OpenAI spend alert"));

    SpendAlert alert;
    alert.setThresholdAmount(kThousandDollars);
    alert.setNotificationChannel(channel);
    return alert;
}

} // namespace

// Coverage for the spend alerts and data-retention settings (#106, under #28).
// Offline: every request goes to the local stub server.
class TestSpendAlerts : public QObject
{
    Q_OBJECT
private slots:
    void alertsRoundTripThroughJson();
    void aFreshAlertCarriesTheOnlyValuesTheApiAccepts();
    void aZeroThresholdIsSentRatherThanOmitted();
    void createSendsOnlyTheRequestFields();
    void listAndDeleteAtTheOrganizationScope();
    void theProjectScopeMirrorsTheOrganizationOne_data();
    void theProjectScopeMirrorsTheOrganizationOne();
    void retentionRoundTripsThroughJson();
    void readingAndWritingRetentionUseDifferentFieldNames();
    void aProjectCanDeferToTheOrganization();
};

void TestSpendAlerts::alertsRoundTripThroughJson()
{
    const SpendAlert alert = SpendAlert::fromJson(QJsonDocument::fromJson(alertJson()).object());

    QCOMPARE(alert.id(), QStringLiteral("alert_abc123"));
    QCOMPARE(alert.object(), QStringLiteral("organization.spend_alert"));
    // Cents, not dollars. A hundredfold error here is an alert that never fires.
    QCOMPARE(alert.thresholdAmount(), kThousandDollars);
    QCOMPARE(alert.currency(), QStringLiteral("USD"));
    QCOMPARE(alert.interval(), QStringLiteral("month"));
    QCOMPARE(alert.notificationChannel().recipients(),
             QStringList {QStringLiteral("finance@example.com")});
    QCOMPARE(alert.notificationChannel().subjectPrefix(), QStringLiteral("OpenAI spend alert"));
    QVERIFY(!alert.isDeleted());

    QCOMPARE(SpendAlert::fromJson(alert.toJson()), alert);

    // The project scope is the same type, differing only in `object` -- which is
    // the whole reason there is one class rather than two.
    const SpendAlert projectAlert = SpendAlert::fromJson(
            QJsonDocument::fromJson(alertJson("project.spend_alert")).object());
    QCOMPARE(projectAlert.object(), QStringLiteral("project.spend_alert"));
    QCOMPARE(projectAlert.thresholdAmount(), alert.thresholdAmount());
}

void TestSpendAlerts::aFreshAlertCarriesTheOnlyValuesTheApiAccepts()
{
    // Both of these are required by the API and have exactly one legal value
    // today, so defaulting them means a create built from scratch is accepted
    // rather than refused for a field the caller never had a choice about.
    const SpendAlert fresh;
    QCOMPARE(fresh.currency(), QStringLiteral("USD"));
    QCOMPARE(fresh.interval(), QStringLiteral("month"));
    QCOMPARE(fresh.notificationChannel().type(), QStringLiteral("email"));
}

void TestSpendAlerts::aZeroThresholdIsSentRatherThanOmitted()
{
    // 0 is a legal threshold -- an alert on the first cent of the month -- so it
    // goes on the wire. Omitting it would turn a valid request into one the
    // server refuses for a missing required field.
    SpendAlert alert = sampleAlert();
    alert.setThresholdAmount(0);
    QVERIFY(alert.toJson().contains(QStringLiteral("threshold_amount")));
    QCOMPARE(alert.toJson().value(QStringLiteral("threshold_amount")).toInteger(), qint64(0));

    // And it survives a decode as itself rather than as "unset".
    const SpendAlertList page
            = SpendAlertList::fromJson(QJsonDocument::fromJson(alertPage()).object());
    QCOMPARE(page.size(), 2);
    QCOMPARE(page.data.at(1).thresholdAmount(), qint64(0));
}

void TestSpendAlerts::createSendsOnlyTheRequestFields()
{
    StubServer server(alertJson());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    // Read back from a server first, so the value carries an id and an object
    // that the request must not send back as though they were being asked for.
    SpendAlert alert = SpendAlert::fromJson(QJsonDocument::fromJson(alertJson()).object());

    const auto reply = awaited(organization.createSpendAlert(alert));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(server.requestLine(), "POST /v1/organization/spend_alerts HTTP/1.1");
    QVERIFY(server.requestHeaders().contains("Authorization: Bearer sk-admin-test"));

    // Exactly the four fields the API asks for: the server assigns id and
    // object, and sending them back would be describing the resource rather
    // than requesting it.
    QCOMPARE(server.requestBody(),
             R"({"currency":"USD","interval":"month","notification_channel":{"recipients":)"
             R"(["finance@example.com"],"subject_prefix":"OpenAI spend alert","type":"email"},)"
             R"("threshold_amount":100000})");
}

void TestSpendAlerts::listAndDeleteAtTheOrganizationScope()
{
    StubServer listed(alertPage());
    Organization organization(listed.baseUrl(), QStringLiteral("sk-admin-test"));

    QtOpenAi::Client::ListParams params;
    params.limit = 5;

    const auto page = awaited(organization.listSpendAlerts(params));
    QVERIFY(page);
    QVERIFY2(page->isSuccess(), qPrintable(page->error().message()));
    QCOMPARE(listed.requestLine(), "GET /v1/organization/spend_alerts?limit=5 HTTP/1.1");
    QCOMPARE(page->alerts().size(), 2);
    QCOMPARE(page->alerts().data.at(1).notificationChannel().recipients().size(), 2);

    StubServer removed(R"({"id":"alert_abc123","object":"organization.spend_alert.deleted",
        "deleted":true})");
    Organization other(removed.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(other.deleteSpendAlert(QStringLiteral("alert_abc123")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(removed.requestLine(), "DELETE /v1/organization/spend_alerts/alert_abc123 HTTP/1.1");
    QVERIFY(reply->alert().isDeleted());
    QCOMPARE(reply->alert().id(), QStringLiteral("alert_abc123"));
}

void TestSpendAlerts::theProjectScopeMirrorsTheOrganizationOne_data()
{
    QTest::addColumn<int>("endpoint");
    QTest::addColumn<QByteArray>("line");

    // Every operation exists at both scopes, and the project's nest under
    // /organization/projects/{id} -- unlike the role endpoints, which is why
    // these are named methods rather than a RoleScope argument.
    QTest::newRow("org get") << 0
                             << QByteArray("GET /v1/organization/spend_alerts/alert_1 HTTP/1.1");
    QTest::newRow("org update") << 1
                                << QByteArray(
                                           "POST /v1/organization/spend_alerts/alert_1 HTTP/1.1");
    QTest::newRow("project list")
            << 2 << QByteArray("GET /v1/organization/projects/proj_1/spend_alerts HTTP/1.1");
    QTest::newRow("project get")
            << 3
            << QByteArray("GET /v1/organization/projects/proj_1/spend_alerts/alert_1 HTTP/1.1");
    QTest::newRow("project create")
            << 4 << QByteArray("POST /v1/organization/projects/proj_1/spend_alerts HTTP/1.1");
    QTest::newRow("project update")
            << 5
            << QByteArray("POST /v1/organization/projects/proj_1/spend_alerts/alert_1 HTTP/1.1");
    QTest::newRow("project delete")
            << 6
            << QByteArray("DELETE /v1/organization/projects/proj_1/spend_alerts/alert_1 HTTP/1.1");
    QTest::newRow("org retention")
            << 7 << QByteArray("GET /v1/organization/data_retention HTTP/1.1");
    QTest::newRow("project retention")
            << 8 << QByteArray("GET /v1/organization/projects/proj_1/data_retention HTTP/1.1");
}

void TestSpendAlerts::theProjectScopeMirrorsTheOrganizationOne()
{
    QFETCH(int, endpoint);
    QFETCH(QByteArray, line);

    StubServer server(alertJson());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));
    const QString project = QStringLiteral("proj_1");
    const QString alertId = QStringLiteral("alert_1");

    switch (endpoint) {
    case 0:
        QVERIFY(awaited(organization.getSpendAlert(alertId)));
        break;
    case 1:
        QVERIFY(awaited(organization.updateSpendAlert(alertId, sampleAlert())));
        break;
    case 2:
        QVERIFY(awaited(organization.listProjectSpendAlerts(project)));
        break;
    case 3:
        QVERIFY(awaited(organization.getProjectSpendAlert(project, alertId)));
        break;
    case 4:
        QVERIFY(awaited(organization.createProjectSpendAlert(project, sampleAlert())));
        break;
    case 5:
        QVERIFY(awaited(organization.updateProjectSpendAlert(project, alertId, sampleAlert())));
        break;
    case 6:
        QVERIFY(awaited(organization.deleteProjectSpendAlert(project, alertId)));
        break;
    case 7:
        QVERIFY(awaited(organization.getDataRetention()));
        break;
    default:
        QVERIFY(awaited(organization.getProjectDataRetention(project)));
        break;
    }

    QCOMPARE(server.requestLine(), line);
}

void TestSpendAlerts::retentionRoundTripsThroughJson()
{
    const DataRetention retention = DataRetention::fromJson(
            QJsonDocument::fromJson(R"({"object":"organization.data_retention",
                "type":"modified_abuse_monitoring"})")
                    .object());

    QCOMPARE(retention.object(), QStringLiteral("organization.data_retention"));
    QCOMPARE(retention.type(), QStringLiteral("modified_abuse_monitoring"));
    QVERIFY(!retention.isOrganizationDefault());
    QCOMPARE(DataRetention::fromJson(retention.toJson()), retention);

    // A policy this build has never heard of survives as itself. On a compliance
    // setting that is the difference between reporting the strictest policy and
    // reporting the default.
    const DataRetention future = DataRetention::fromJson(
            QJsonDocument::fromJson(R"({"object":"organization.data_retention",
                "type":"sealed_enclave_only"})")
                    .object());
    QCOMPARE(future.type(), QStringLiteral("sealed_enclave_only"));
    QCOMPARE(DataRetention::fromJson(future.toJson()), future);
}

void TestSpendAlerts::readingAndWritingRetentionUseDifferentFieldNames()
{
    StubServer server(R"({"object":"organization.data_retention",
        "type":"zero_data_retention"})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply
            = awaited(organization.setDataRetention(QStringLiteral("zero_data_retention")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(server.requestLine(), "POST /v1/organization/data_retention HTTP/1.1");

    // The update body says `retention_type`; the resource reports `type`. That
    // asymmetry is the API's, and sending the resource's own shape back would
    // be a request the server accepts and quietly ignores.
    QCOMPARE(server.requestBody(), R"({"retention_type":"zero_data_retention"})");
    QCOMPARE(reply->retention().type(), QStringLiteral("zero_data_retention"));
}

void TestSpendAlerts::aProjectCanDeferToTheOrganization()
{
    StubServer server(R"({"object":"project.data_retention","type":"organization_default"})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(organization.setProjectDataRetention(
            QStringLiteral("proj_1"), QStringLiteral("organization_default")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(server.requestLine(), "POST /v1/organization/projects/proj_1/data_retention HTTP/1.1");
    QCOMPARE(server.requestBody(), R"({"retention_type":"organization_default"})");

    // Two of the six values are the project scope's alone, and this is the one
    // worth asking about: it means "whatever the organization says", not a
    // policy of its own.
    QVERIFY(reply->retention().isOrganizationDefault());
}

QTEST_MAIN(TestSpendAlerts)
#include "tst_spendalerts.moc"
