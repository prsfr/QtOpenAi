// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QUrlQuery>

namespace QtOpenAi {
namespace Admin {

// The filters of GET /organization/audit_logs.
//
// A struct rather than eleven parameters, as Admin::UsageQuery is and for the
// same reason: this is a surface where being silently wrong is easy. A filter
// the server does not recognise is not an error — it is ignored, and the caller
// gets a perfectly valid page of the wrong events, which for an audit trail is
// worse than no answer. So the tests assert the query string that goes on the
// wire, not the reply.
//
//     Admin::AuditLogQuery query;
//     query.effectiveAtGte = QDateTime::currentSecsSinceEpoch() - 24 * 3600;
//     query.eventTypes = {QStringLiteral("project.archived")};
//
// **Every list filter is sent with `[]` in its name** — `project_ids[]=a&
// project_ids[]=b` — because that is how the API spells them, brackets included,
// in the parameter names themselves.
//
// **The time bounds are sent as `effective_at[gt]`** and friends. The API models
// this one parameter as an object; the only other object-valued query parameter
// in the whole specification is annotated `style: deepObject`, which is exactly
// this bracketed spelling, and it matches the bracket convention the list
// filters above use openly.
struct QTOPENAI_ADMIN_EXPORT AuditLogQuery
{
    // Unix seconds, all four independent and all optional; 0 means unset.
    // `gte`/`lte` include the boundary second, `gt`/`lt` exclude it — which for
    // a log with several events in the same second is a real difference.
    qint64 effectiveAtGt = 0;
    qint64 effectiveAtGte = 0;
    qint64 effectiveAtLt = 0;
    qint64 effectiveAtLte = 0;

    // Restrict to events scoped to these projects; empty means all.
    QStringList projectIds;

    // Restrict to these event types, e.g. "project.archived". Strings rather
    // than an enum, matching Core::AuditLog::type(): filtering on a type this
    // build has never heard of has to reach the server rather than fail here.
    QStringList eventTypes;

    // Restrict to who acted. `actorIds` are user or service-account ids;
    // `actorEmails` the addresses, which is usually what an investigation has.
    QStringList actorIds;
    QStringList actorEmails;

    // Restrict to what was acted on — matched against the payload's `id`, which
    // Core::AuditLog exposes as resourceId().
    QStringList resourceIds;

    // Exclude events from projects the organization does not own. False (the
    // default) is the server's own default and is not sent.
    bool tenantOnly = false;

    // Page size (-1 leaves the server default of 20) and the cursors of a
    // previous Core::AuditLogList.
    int limit = -1;
    QString after;
    QString before;

    QUrlQuery toQuery() const;
};

} // namespace Admin
} // namespace QtOpenAi
