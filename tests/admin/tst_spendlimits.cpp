// SPDX-License-Identifier: MIT
#include <QtOpenAi/Admin/Organization.h>
#include <QtOpenAi/Core/SpendLimit.h>

#include <QtTest/QtTest>

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Admin;

namespace {

QByteArray limitJson(const char *object = "organization.spend_limit",
                     const char *status = "inactive")
{
    return QByteArray(R"({"object":")") + object
           + R"(","threshold_amount":100000,"currency":"USD","interval":"month",
        "enforcement":{"status":")"
           + status + R"("}})";
}

SpendLimit sampleLimit()
{
    SpendLimit limit;
    limit.setThresholdAmount(100000); // $1,000.00
    return limit;
}

} // namespace

// Coverage for the hard spend limits (/organization/spend_limit and the project
// mirror), the sibling of the spend alerts in #106. Offline: every request goes
// to the local stub server.
class TestSpendLimits : public QObject
{
    Q_OBJECT
private slots:
    void limitsRoundTripThroughJson();
    void enforcementIsReadNotWritten();
    void aFreshLimitCarriesTheOnlyValuesTheApiAccepts();
    void anUnsetThresholdIsOmittedRatherThanSentAsZero();
    void setSendsOnlyTheThreeWritableFields();
    void deletingIsHowSpendingBecomesUnlimited();
    void everyPathIsComposedAtBothScopes_data();
    void everyPathIsComposedAtBothScopes();
};

void TestSpendLimits::limitsRoundTripThroughJson()
{
    const SpendLimit limit = SpendLimit::fromJson(QJsonDocument::fromJson(limitJson()).object());

    QCOMPARE(limit.object(), QStringLiteral("organization.spend_limit"));
    // Cents, as the alert's threshold is.
    QCOMPARE(limit.thresholdAmount(), qint64(100000));
    QCOMPARE(limit.currency(), QStringLiteral("USD"));
    QCOMPARE(limit.interval(), QStringLiteral("month"));
    QCOMPARE(limit.enforcementStatus(), QStringLiteral("inactive"));
    QVERIFY(!limit.isDeleted());
    QCOMPARE(SpendLimit::fromJson(limit.toJson()), limit);

    // One type for both scopes, differing only in `object` -- as the alerts do.
    const SpendLimit projectLimit = SpendLimit::fromJson(
            QJsonDocument::fromJson(limitJson("project.spend_limit")).object());
    QCOMPARE(projectLimit.object(), QStringLiteral("project.spend_limit"));
    QCOMPARE(projectLimit.thresholdAmount(), limit.thresholdAmount());
}

void TestSpendLimits::enforcementIsReadNotWritten()
{
    // "A limit exists" and "a limit is currently refusing requests" are
    // different facts, and this is the one an operator is paged about.
    const SpendLimit quiet = SpendLimit::fromJson(
            QJsonDocument::fromJson(limitJson("organization.spend_limit", "inactive")).object());
    QVERIFY(!quiet.isEnforcing());
    QCOMPARE(quiet.thresholdAmount(), qint64(100000)); // configured all the same

    const SpendLimit biting = SpendLimit::fromJson(
            QJsonDocument::fromJson(limitJson("organization.spend_limit", "enforcing")).object());
    QVERIFY(biting.isEnforcing());

    // A status this build has never heard of is neither, rather than decaying to
    // "not enforcing" -- which would report an outage as business as usual.
    const SpendLimit future = SpendLimit::fromJson(
            QJsonDocument::fromJson(limitJson("organization.spend_limit", "grace_period"))
                    .object());
    QVERIFY(!future.isEnforcing());
    QCOMPARE(future.enforcementStatus(), QStringLiteral("grace_period"));
    QCOMPARE(SpendLimit::fromJson(future.toJson()), future);
}

void TestSpendLimits::aFreshLimitCarriesTheOnlyValuesTheApiAccepts()
{
    const SpendLimit fresh;
    QCOMPARE(fresh.currency(), QStringLiteral("USD"));
    QCOMPARE(fresh.interval(), QStringLiteral("month"));
    // Nothing is enforcing until a server says so.
    QVERIFY(fresh.enforcementStatus().isEmpty());
    QVERIFY(!fresh.isEnforcing());
}

void TestSpendLimits::anUnsetThresholdIsOmittedRatherThanSentAsZero()
{
    // The opposite of Core::SpendAlert, and deliberately so: the API's minimum
    // here is 1, so a zero is an unset limit rather than a limit of nothing.
    // Writing it would ask the server to permit no spending at all.
    const SpendLimit unset;
    QCOMPARE(unset.thresholdAmount(), qint64(0));
    QVERIFY(!unset.toJson().contains(QStringLiteral("threshold_amount")));

    SpendLimit set = sampleLimit();
    QVERIFY(set.toJson().contains(QStringLiteral("threshold_amount")));
}

void TestSpendLimits::setSendsOnlyTheThreeWritableFields()
{
    StubServer server(limitJson());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    // Read back from a server first, so the value carries an object and an
    // enforcement state that the request must not send back as settings.
    SpendLimit limit = SpendLimit::fromJson(QJsonDocument::fromJson(limitJson()).object());

    const auto reply = awaited(organization.setSpendLimit(limit));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(server.requestLine(), "POST /v1/organization/spend_limit HTTP/1.1");
    QVERIFY(server.requestHeaders().contains("Authorization: Bearer sk-admin-test"));

    // Exactly the three the API asks for. Whether the limit is biting is the
    // server's report, not a setting, so it never goes out.
    QCOMPARE(server.requestBody(),
             R"({"currency":"USD","interval":"month","threshold_amount":100000})");
    QCOMPARE(reply->limit().thresholdAmount(), qint64(100000));
}

void TestSpendLimits::deletingIsHowSpendingBecomesUnlimited()
{
    StubServer server(R"({"object":"organization.spend_limit.deleted","deleted":true})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(organization.deleteSpendLimit());
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(server.requestLine(), "DELETE /v1/organization/spend_limit HTTP/1.1");

    // There is no "no limit" to write, so the acknowledgement carries no
    // threshold at all -- removing it is the only way back to unlimited.
    QVERIFY(reply->limit().isDeleted());
    QCOMPARE(reply->limit().thresholdAmount(), qint64(0));
    QVERIFY(!reply->limit().isEnforcing());
}

void TestSpendLimits::everyPathIsComposedAtBothScopes_data()
{
    QTest::addColumn<int>("endpoint");
    QTest::addColumn<QByteArray>("line");

    // A scope has one limit or none, so no id appears anywhere -- and the
    // project mirror nests under /organization/projects/{id} like the alerts.
    QTest::newRow("org get") << 0 << QByteArray("GET /v1/organization/spend_limit HTTP/1.1");
    QTest::newRow("org delete") << 1 << QByteArray("DELETE /v1/organization/spend_limit HTTP/1.1");
    QTest::newRow("project get")
            << 2 << QByteArray("GET /v1/organization/projects/proj_1/spend_limit HTTP/1.1");
    QTest::newRow("project set")
            << 3 << QByteArray("POST /v1/organization/projects/proj_1/spend_limit HTTP/1.1");
    QTest::newRow("project delete")
            << 4 << QByteArray("DELETE /v1/organization/projects/proj_1/spend_limit HTTP/1.1");
}

void TestSpendLimits::everyPathIsComposedAtBothScopes()
{
    QFETCH(int, endpoint);
    QFETCH(QByteArray, line);

    StubServer server(limitJson());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));
    const QString project = QStringLiteral("proj_1");

    switch (endpoint) {
    case 0:
        QVERIFY(awaited(organization.getSpendLimit()));
        break;
    case 1:
        QVERIFY(awaited(organization.deleteSpendLimit()));
        break;
    case 2:
        QVERIFY(awaited(organization.getProjectSpendLimit(project)));
        break;
    case 3:
        QVERIFY(awaited(organization.setProjectSpendLimit(project, sampleLimit())));
        break;
    default:
        QVERIFY(awaited(organization.deleteProjectSpendLimit(project)));
        break;
    }

    QCOMPARE(server.requestLine(), line);
}

QTEST_MAIN(TestSpendLimits)
#include "tst_spendlimits.moc"
