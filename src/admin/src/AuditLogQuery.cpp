// SPDX-License-Identifier: MIT
#include "QtOpenAi/Admin/AuditLogQuery.h"

#include "UrlQuery_p.h"

namespace QtOpenAi {
namespace Admin {

namespace {

// Note the keys passed to detail::appendEach() below: the audit-log filters
// carry the `[]` in the parameter name itself, which is how the API spells them
// and is not something the helper adds.

// One bound of the effective_at filter, as `effective_at[gt]=...`. Omitted at 0
// rather than sent: an unset bound and "at the epoch" are different requests,
// and the latter would silently narrow a window nobody meant to narrow.
void addBound(QUrlQuery &query, const QString &comparison, qint64 seconds)
{
    if (seconds > 0)
        query.addQueryItem(QStringLiteral("effective_at[%1]").arg(comparison),
                           QString::number(seconds));
}

} // namespace

QUrlQuery AuditLogQuery::toQuery() const
{
    QUrlQuery query;
    addBound(query, QStringLiteral("gt"), effectiveAtGt);
    addBound(query, QStringLiteral("gte"), effectiveAtGte);
    addBound(query, QStringLiteral("lt"), effectiveAtLt);
    addBound(query, QStringLiteral("lte"), effectiveAtLte);

    Core::detail::appendEach(query, QStringLiteral("project_ids[]"), projectIds);
    Core::detail::appendEach(query, QStringLiteral("event_types[]"), eventTypes);
    Core::detail::appendEach(query, QStringLiteral("actor_ids[]"), actorIds);
    Core::detail::appendEach(query, QStringLiteral("actor_emails[]"), actorEmails);
    Core::detail::appendEach(query, QStringLiteral("resource_ids[]"), resourceIds);

    // Only when true: false is the server's own default, and sending it would
    // add noise to every query for no change in the answer.
    if (tenantOnly)
        query.addQueryItem(QStringLiteral("tenant_only"), QStringLiteral("true"));

    if (limit >= 0)
        query.addQueryItem(QStringLiteral("limit"), QString::number(limit));
    if (!after.isEmpty())
        query.addQueryItem(QStringLiteral("after"), after);
    if (!before.isEmpty())
        query.addQueryItem(QStringLiteral("before"), before);
    return query;
}

} // namespace Admin
} // namespace QtOpenAi
