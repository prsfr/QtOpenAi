// SPDX-License-Identifier: MIT
#include <QtOpenAi/Admin/Organization.h>
#include <QtOpenAi/Core/AuditLog.h>

#include <QtTest/QtTest>

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Admin;

namespace {

// One entry per shape the payloads come in: a create carrying `data`, an update
// carrying `changes_requested`, one that names no resource at all, and one whose
// event type this build has never heard of.
QByteArray auditLogPage()
{
    return R"({"object":"list","data":[
        {"id":"audit_log-1","type":"project.archived","effective_at":1722461446,
         "project":{"id":"proj_abc","name":"Production"},
         "actor":{"type":"api_key","api_key":{"id":"key_1","type":"user",
                  "user":{"id":"user-1","email":"ada@example.com"}}},
         "project.archived":{"id":"proj_abc"}},
        {"id":"audit_log-2","type":"api_key.updated","effective_at":1720804190,
         "actor":{"type":"session","session":{"user":{"id":"user-2",
                  "email":"grace@example.com"},"ip_address":"127.0.0.1",
                  "user_agent":"Mozilla/5.0"}},
         "api_key.updated":{"id":"key_2","changes_requested":{"scopes":["api.model.request"]}}},
        {"id":"audit_log-3","type":"user.added","effective_at":1720804000,
         "actor":{"type":"api_key","api_key":{"id":"key_3","type":"service_account",
                  "service_account":{"id":"sa_1"}}},
         "user.added":{"id":"user-3","data":{"role":"member"}}},
        {"id":"audit_log-4","type":"login.failed","effective_at":1720803000,
         "actor":{"type":"session","session":{"ip_address":"10.0.0.9"}},
         "login.failed":{"error_code":"invalid_credentials","error_message":"nope"}},
        {"id":"audit_log-5","type":"quantum.entangled","effective_at":1720802000,
         "actor":{"type":"session","session":{"user":{"id":"user-9",
                  "email":"future@example.com"}}},
         "quantum.entangled":{"id":"qbit_1","spin":"up",
                              "data":{"partners":["qbit_2"]}}}],
        "first_id":"audit_log-1","last_id":"audit_log-5","has_more":true})";
}

AuditLogList decodedPage()
{
    return AuditLogList::fromJson(QJsonDocument::fromJson(auditLogPage()).object());
}

} // namespace

// Coverage for the organization audit logs (#105, under #28). Offline: every
// request goes to the local stub server.
class TestAuditLogs : public QObject
{
    Q_OBJECT
private slots:
    void thePayloadIsFoundUnderTheEventTypesOwnName_data();
    void thePayloadIsFoundUnderTheEventTypesOwnName();
    void anUnknownEventTypeArrivesWhole();
    void dataAndChangesRequestedAreDifferentClaims();
    void actorsDecodeFromEitherHalfOfTheUnion();
    void entriesRoundTripThroughJson();
    void listSendsTheFilterQuery();
    void anUnfilteredListSendsNoQueryAtAll();
    void theTimeBoundsAreSentAsBracketedComparisons();
};

void TestAuditLogs::thePayloadIsFoundUnderTheEventTypesOwnName_data()
{
    QTest::addColumn<int>("index");
    QTest::addColumn<QString>("type");
    QTest::addColumn<QString>("resourceId");

    // The representative handful #105 asks for. What they have in common is the
    // rule: the payload sits under a key named after `type`, so finding it never
    // needs the type to be known in advance.
    QTest::newRow("archived") << 0 << "project.archived" << "proj_abc";
    QTest::newRow("updated") << 1 << "api_key.updated" << "key_2";
    QTest::newRow("added") << 2 << "user.added" << "user-3";
    // An event about no particular resource still decodes; it just names none.
    QTest::newRow("login failure") << 3 << "login.failed" << QString();
    QTest::newRow("never heard of it") << 4 << "quantum.entangled" << "qbit_1";
}

void TestAuditLogs::thePayloadIsFoundUnderTheEventTypesOwnName()
{
    QFETCH(int, index);
    QFETCH(QString, type);
    QFETCH(QString, resourceId);

    const AuditLog entry = decodedPage().data.at(index);
    QCOMPARE(entry.type(), type);
    QCOMPARE(entry.resourceId(), resourceId);
    QVERIFY(!entry.payload().isEmpty());
    QVERIFY(entry.effectiveAt() > 0);
}

void TestAuditLogs::anUnknownEventTypeArrivesWhole()
{
    // The acceptance criterion of #105. There is no enum to miss it, so an event
    // type added after this build is not a special case -- it reads exactly as
    // well as one the library was written against, fields and all.
    const AuditLog entry = decodedPage().data.at(4);
    QCOMPARE(entry.type(), QStringLiteral("quantum.entangled"));
    QCOMPARE(entry.resourceId(), QStringLiteral("qbit_1"));

    // Including the parts no accessor names: nothing was dropped on the way in.
    QCOMPARE(entry.payload().value(QStringLiteral("spin")).toString(), QStringLiteral("up"));
    QCOMPARE(entry.data().value(QStringLiteral("partners")).toArray().size(), 1);

    // And it survives back out again, under its own name.
    const QJsonObject json = entry.toJson();
    QVERIFY(json.contains(QStringLiteral("quantum.entangled")));
    QCOMPARE(AuditLog::fromJson(json), entry);
}

void TestAuditLogs::dataAndChangesRequestedAreDifferentClaims()
{
    const AuditLogList page = decodedPage();

    // `changes_requested` is what an update asked to change...
    const AuditLog updated = page.data.at(1);
    QCOMPARE(updated.changesRequested().value(QStringLiteral("scopes")).toArray().at(0).toString(),
             QStringLiteral("api.model.request"));
    QVERIFY(updated.data().isEmpty());

    // ...and `data` is what a creation was given. Conflating them would report
    // an attempted change as an accomplished one.
    const AuditLog added = page.data.at(2);
    QCOMPARE(added.data().value(QStringLiteral("role")).toString(), QStringLiteral("member"));
    QVERIFY(added.changesRequested().isEmpty());
}

void TestAuditLogs::actorsDecodeFromEitherHalfOfTheUnion()
{
    const AuditLogList page = decodedPage();

    // An API key belonging to a person: the person is still the answer to "who".
    const AuditLogActor viaKey = page.data.at(0).actor();
    QVERIFY(viaKey.isApiKey());
    QVERIFY(!viaKey.isSession());
    QCOMPARE(viaKey.apiKeyId(), QStringLiteral("key_1"));
    QCOMPARE(viaKey.user().email(), QStringLiteral("ada@example.com"));

    // A browser session: same question, same shape of answer, plus where from.
    const AuditLogActor viaSession = page.data.at(1).actor();
    QVERIFY(viaSession.isSession());
    QCOMPARE(viaSession.user().email(), QStringLiteral("grace@example.com"));
    QCOMPARE(viaSession.ipAddress(), QStringLiteral("127.0.0.1"));
    // Absent from the API's schema but present in its responses, so it is kept
    // rather than dropped -- an audit trail is the wrong place to lose evidence.
    QCOMPARE(viaSession.userAgent(), QStringLiteral("Mozilla/5.0"));

    // A service-account key has no person behind it, and the empty user is the
    // answer rather than a decode that failed.
    const AuditLogActor viaServiceAccount = page.data.at(2).actor();
    QVERIFY(viaServiceAccount.isApiKey());
    QVERIFY(viaServiceAccount.isServiceAccount());
    QCOMPARE(viaServiceAccount.serviceAccountId(), QStringLiteral("sa_1"));
    QVERIFY(viaServiceAccount.user().isEmpty());
}

void TestAuditLogs::entriesRoundTripThroughJson()
{
    const AuditLogList page = decodedPage();
    QCOMPARE(page.size(), 5);
    QVERIFY(page.hasMore);
    QCOMPARE(page.firstId, QStringLiteral("audit_log-1"));

    for (const AuditLog &entry : page.data)
        QVERIFY2(AuditLog::fromJson(entry.toJson()) == entry, qPrintable(entry.id()));

    // The project wrapper is flattened on the way in and rebuilt on the way out.
    const AuditLog scoped = page.data.at(0);
    QCOMPARE(scoped.projectId(), QStringLiteral("proj_abc"));
    QCOMPARE(scoped.projectName(), QStringLiteral("Production"));
    QCOMPARE(scoped.toJson()
                     .value(QStringLiteral("project"))
                     .toObject()
                     .value(QStringLiteral("name"))
                     .toString(),
             QStringLiteral("Production"));

    // An entry scoped to no project writes no project back.
    QVERIFY(page.data.at(3).projectId().isEmpty());
    QVERIFY(!page.data.at(3).toJson().contains(QStringLiteral("project")));
}

void TestAuditLogs::listSendsTheFilterQuery()
{
    StubServer server(auditLogPage());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    AuditLogQuery query;
    query.projectIds = {QStringLiteral("proj_abc"), QStringLiteral("proj_def")};
    query.eventTypes = {QStringLiteral("project.archived")};
    query.actorEmails = {QStringLiteral("ada@example.com")};
    query.resourceIds = {QStringLiteral("proj_abc")};
    query.tenantOnly = true;
    query.limit = 10;
    query.after = QStringLiteral("audit_log-1");

    const auto reply = awaited(organization.listAuditLogs(query));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    // Asserted on the wire rather than through the reply: a filter the server
    // does not recognise is ignored, not refused, so a wrong query comes back as
    // a valid page of the wrong events. Note the `[]` in every list filter's
    // name -- that is how the API spells them, brackets and all -- and that a
    // repeated filter repeats the key rather than joining with a comma.
    QCOMPARE(server.requestLine(),
             "GET /v1/organization/audit_logs?project_ids[]=proj_abc&project_ids[]=proj_def"
             "&event_types[]=project.archived&actor_emails[]=ada@example.com"
             "&resource_ids[]=proj_abc&tenant_only=true&limit=10&after=audit_log-1 HTTP/1.1");
    QCOMPARE(reply->auditLogs().size(), 5);
}

void TestAuditLogs::anUnfilteredListSendsNoQueryAtAll()
{
    StubServer server(auditLogPage());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    QVERIFY(awaited(organization.listAuditLogs()));
    // No stray `tenant_only=false` or `limit=-1`: every default is the server's
    // to apply, and sending one back is how a default silently becomes a filter.
    QCOMPARE(server.requestLine(), "GET /v1/organization/audit_logs HTTP/1.1");
}

void TestAuditLogs::theTimeBoundsAreSentAsBracketedComparisons()
{
    StubServer server(auditLogPage());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    AuditLogQuery query;
    query.effectiveAtGt = 1720000000;
    query.effectiveAtLte = 1730000000;

    QVERIFY(awaited(organization.listAuditLogs(query)));
    // The API models effective_at as an object, and the one other object-valued
    // query parameter in its specification is annotated `style: deepObject` --
    // which is this bracketed spelling, and matches the brackets the list
    // filters carry openly.
    QCOMPARE(server.requestLine(), "GET /v1/organization/audit_logs?effective_at[gt]=1720000000"
                                   "&effective_at[lte]=1730000000 HTTP/1.1");

    // The two bounds nobody set are absent rather than sent as 0, which would
    // have narrowed the window to nothing.
    QVERIFY(!server.requestLine().contains("gte"));
    QVERIFY(!server.requestLine().contains("[lt]"));
}

QTEST_MAIN(TestAuditLogs)
#include "tst_auditlogs.moc"
