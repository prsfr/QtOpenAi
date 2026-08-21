// SPDX-License-Identifier: MIT
#include <QtOpenAi/Admin/Organization.h>
#include <QtOpenAi/Client/PageWalker.h>

#include <QtTest/QtTest>

#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Admin;
using QtOpenAi::Client::ListParams;
using QtOpenAi::Client::PageWalker;
using QtOpenAi::Client::PageWalkerBase;

namespace {

// A CursorPage: paginated by the opaque `next`, not by an item id.
QByteArray rolePage(const char *id, const char *next, bool hasMore)
{
    return QByteArray(R"({"object":"list","data":[{"object":"role","id":")") + id
           + R"(","name":"Role","permissions":["api.groups.read"],
                 "resource_type":"api.organization","predefined_role":false}],
        "has_more":)"
           + (hasMore ? "true" : "false") + R"(,"next":)"
           + (next ? (QByteArray("\"") + next + "\"") : QByteArray("null")) + "}";
}

// A BucketPage: paginated by `next_page`, and the cursor goes back as `page`
// rather than `after`.
QByteArray usagePage(const char *nextPage, bool hasMore)
{
    return QByteArray(R"({"object":"page","data":[
        {"object":"bucket","start_time":1730419200,"end_time":1730505600,"results":[
            {"object":"organization.usage.completions.result","input_tokens":10,
             "output_tokens":5,"num_model_requests":1}]}],
        "has_more":)")
           + (hasMore ? "true" : "false") + R"(,"next_page":)"
           + (nextPage ? (QByteArray("\"") + nextPage + "\"") : QByteArray("null")) + "}";
}

// A ListPage behind a query type that is not ListParams.
QByteArray auditPage(const char *id, bool hasMore)
{
    return QByteArray(R"({"object":"list","data":[{"id":")") + id
           + R"(","type":"project.archived","effective_at":1722461446,
                 "project.archived":{"id":"proj_abc"}}],
        "first_id":")"
           + id + R"(","last_id":")" + id + R"(","has_more":)" + (hasMore ? "true" : "false") + "}";
}

} // namespace

// Coverage for walking the two page shapes PageWalker could not drive before
// #120, and the one that has the right shape but its own query type. Offline:
// every request goes to the local stub server.
//
// This lives under tests/admin rather than beside the ListPage cases in
// tst_client_pagination.cpp because the endpoints that produce these shapes are
// all administration ones, and a client test should not link the admin module.
class TestAdminPagination : public QObject
{
    Q_OBJECT
private slots:
    void walksACursorPageByItsOpaqueNext();
    void walksABucketPageByItsPageToken();
    void walksAListPageBehindItsOwnQueryType();
    void carriesTheCallersFiltersOntoEveryPage();
    void aPageThatClaimsMoreWithoutACursorStops();
    void surfacesRequestFailure();
};

void TestAdminPagination::walksACursorPageByItsOpaqueNext()
{
    StubServer server(QList<StubServer::Response> {
            {rolePage("role_a", "cursor_two", true)},
            {rolePage("role_b", nullptr, false)},
    });
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    // Roles take ListParams, so the default third template argument still fits;
    // what was missing before #120 was the *reading* half -- this page carries
    // `next`, and the walker used to look for `last_id` and find nothing.
    auto *walker = new PageWalker<RoleListReply, OrganizationRoleList>(
            [&organization](const ListParams &p) { return organization.listRoles({}, p); });
    walker->setAutoDelete(false);

    QStringList seen;
    walker->setPageHandler([&seen](const OrganizationRoleList &page) {
        for (const OrganizationRole &role : page.data)
            seen.append(role.id());
    });
    QSignalSpy finishedSpy(walker, &PageWalkerBase::finished);

    walker->start();
    QVERIFY(finishedSpy.wait(5000));

    QCOMPARE(seen, QStringList({QStringLiteral("role_a"), QStringLiteral("role_b")}));
    QCOMPARE(server.requestCount(), 2);
    // The opaque cursor went back as `after`, which is where this family carries
    // it -- not as the id of the last role on the page.
    QVERIFY2(server.requestLines().at(1).contains("after=cursor_two"),
             server.requestLines().at(1).constData());
    delete walker;
}

void TestAdminPagination::walksABucketPageByItsPageToken()
{
    StubServer server(QList<StubServer::Response> {
            {usagePage("page_two", true)},
            {usagePage(nullptr, false)},
    });
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    UsageQuery query;
    query.startTime = 1730419200;

    // The case that needs all three template arguments: a different page shape
    // *and* a different query type, whose cursor field is `page`.
    auto *walker = new PageWalker<UsageReply, UsagePage, UsageQuery>(
            [&organization](const UsageQuery &q) {
                return organization.usage(Organization::UsageKind::Completions, q);
            },
            query);
    walker->setAutoDelete(false);

    int buckets = 0;
    walker->setPageHandler([&buckets](const UsagePage &page) { buckets += page.size(); });
    QSignalSpy finishedSpy(walker, &PageWalkerBase::finished);

    walker->start();
    QVERIFY(finishedSpy.wait(5000));

    QCOMPARE(buckets, 2);
    QCOMPARE(server.requestCount(), 2);
    // `page=`, not `after=`. Sending `after` here would have re-requested the
    // first bucket for ever, which is the failure this test exists to prevent.
    QVERIFY2(server.requestLines().at(1).contains("page=page_two"),
             server.requestLines().at(1).constData());
    QVERIFY(!server.requestLines().at(1).contains("after="));
    delete walker;
}

void TestAdminPagination::walksAListPageBehindItsOwnQueryType()
{
    StubServer server(QList<StubServer::Response> {
            {auditPage("audit_log-1", true)},
            {auditPage("audit_log-2", false)},
    });
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    // The audit log has the *right* page shape and still could not be walked:
    // listAuditLogs takes AuditLogQuery rather than ListParams, and the walker
    // used to insist on the latter.
    auto *walker = new PageWalker<AuditLogListReply, AuditLogList, AuditLogQuery>(
            [&organization](const AuditLogQuery &q) { return organization.listAuditLogs(q); });
    walker->setAutoDelete(false);

    QStringList seen;
    walker->setPageHandler([&seen](const AuditLogList &page) {
        for (const AuditLog &entry : page.data)
            seen.append(entry.id());
    });
    QSignalSpy finishedSpy(walker, &PageWalkerBase::finished);

    walker->start();
    QVERIFY(finishedSpy.wait(5000));

    QCOMPARE(seen, QStringList({QStringLiteral("audit_log-1"), QStringLiteral("audit_log-2")}));
    QVERIFY2(server.requestLines().at(1).contains("after=audit_log-1"),
             server.requestLines().at(1).constData());
    delete walker;
}

void TestAdminPagination::carriesTheCallersFiltersOntoEveryPage()
{
    StubServer server(QList<StubServer::Response> {
            {auditPage("audit_log-1", true)},
            {auditPage("audit_log-2", false)},
    });
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    AuditLogQuery query;
    query.eventTypes = {QStringLiteral("project.archived")};
    query.limit = 1;

    auto *walker = new PageWalker<AuditLogListReply, AuditLogList, AuditLogQuery>(
            [&organization](const AuditLogQuery &q) { return organization.listAuditLogs(q); },
            query);
    walker->setAutoDelete(false);
    QSignalSpy finishedSpy(walker, &PageWalkerBase::finished);

    walker->start();
    QVERIFY(finishedSpy.wait(5000));

    // Only the cursor field is overwritten between pages. A walk that dropped
    // the filters would quietly widen to the whole log on page two -- which for
    // an audit trail is worse than stopping.
    QCOMPARE(server.requestCount(), 2);
    for (const QByteArray &line : server.requestLines()) {
        QVERIFY2(line.contains("event_types[]=project.archived"), line.constData());
        QVERIFY2(line.contains("limit=1"), line.constData());
    }
    delete walker;
}

void TestAdminPagination::aPageThatClaimsMoreWithoutACursorStops()
{
    // has_more is true but `next` is null. There is nothing to advance on, so
    // the walk ends rather than re-requesting the same page for ever.
    StubServer server(QByteArray(rolePage("role_a", nullptr, true)));
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    auto *walker = new PageWalker<RoleListReply, OrganizationRoleList>(
            [&organization](const ListParams &p) { return organization.listRoles({}, p); });
    walker->setAutoDelete(false);
    QSignalSpy finishedSpy(walker, &PageWalkerBase::finished);

    walker->start();
    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(server.requestCount(), 1);
    QCOMPARE(walker->pageCount(), 1);
    delete walker;
}

void TestAdminPagination::surfacesRequestFailure()
{
    StubServer server(QList<StubServer::Response> {
            {usagePage("page_two", true)},
            {QByteArray(R"({"error":{"message":"nope","type":"server_error"}})"), 500},
    });
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    UsageQuery query;
    query.startTime = 1730419200;

    auto *walker = new PageWalker<UsageReply, UsagePage, UsageQuery>(
            [&organization](const UsageQuery &q) {
                return organization.usage(Organization::UsageKind::Completions, q);
            },
            query);
    walker->setAutoDelete(false);

    int buckets = 0;
    walker->setPageHandler([&buckets](const UsagePage &page) { buckets += page.size(); });
    QSignalSpy failedSpy(walker, &PageWalkerBase::failed);
    QSignalSpy finishedSpy(walker, &PageWalkerBase::finished);

    walker->start();
    QVERIFY(failedSpy.wait(5000));

    // The page handled before the failure stays delivered, and finished() does
    // not also fire -- a failed walk is not a completed one.
    QCOMPARE(buckets, 1);
    QCOMPARE(finishedSpy.count(), 0);
    QVERIFY(walker->isFinished());
    delete walker;
}

QTEST_MAIN(TestAdminPagination)
#include "tst_pagination.moc"
