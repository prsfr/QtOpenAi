// SPDX-License-Identifier: MIT
#include "QtOpenAi/Admin/UsageQuery.h"

#include "UrlQuery_p.h"

namespace QtOpenAi {
namespace Admin {

QUrlQuery UsageQuery::toQuery() const
{
    QUrlQuery query;
    // Written even at 0, unlike every other field here: start_time is required,
    // and a request that omits it should come back as the server's error about
    // a missing parameter rather than as a report of the wrong window.
    query.addQueryItem(QStringLiteral("start_time"), QString::number(startTime));
    if (endTime > 0)
        query.addQueryItem(QStringLiteral("end_time"), QString::number(endTime));
    if (!bucketWidth.isEmpty())
        query.addQueryItem(QStringLiteral("bucket_width"), bucketWidth);
    Core::detail::appendEach(query, QStringLiteral("group_by"), groupBy);
    if (limit >= 0)
        query.addQueryItem(QStringLiteral("limit"), QString::number(limit));
    if (!page.isEmpty())
        query.addQueryItem(QStringLiteral("page"), page);
    Core::detail::appendEach(query, QStringLiteral("project_ids"), projectIds);
    Core::detail::appendEach(query, QStringLiteral("user_ids"), userIds);
    Core::detail::appendEach(query, QStringLiteral("api_key_ids"), apiKeyIds);
    Core::detail::appendEach(query, QStringLiteral("models"), models);
    if (batch)
        query.addQueryItem(QStringLiteral("batch"),
                           *batch ? QStringLiteral("true") : QStringLiteral("false"));
    return query;
}

} // namespace Admin
} // namespace QtOpenAi
