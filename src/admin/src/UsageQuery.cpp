// SPDX-License-Identifier: MIT
#include "QtOpenAi/Admin/UsageQuery.h"

namespace QtOpenAi {
namespace Admin {

namespace {

// Array parameters go out as repeated items -- `models=a&models=b` -- which is
// the `form`/explode style of OpenAI's OpenAPI document and what its own clients
// send. Not comma-joined: a model name is free to contain a comma, and one that
// did would split into two filters that match nothing.
void addEach(QUrlQuery &query, const QString &key, const QStringList &values)
{
    for (const QString &value : values)
        query.addQueryItem(key, value);
}

} // namespace

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
    addEach(query, QStringLiteral("group_by"), groupBy);
    if (limit >= 0)
        query.addQueryItem(QStringLiteral("limit"), QString::number(limit));
    if (!page.isEmpty())
        query.addQueryItem(QStringLiteral("page"), page);
    addEach(query, QStringLiteral("project_ids"), projectIds);
    addEach(query, QStringLiteral("user_ids"), userIds);
    addEach(query, QStringLiteral("api_key_ids"), apiKeyIds);
    addEach(query, QStringLiteral("models"), models);
    if (batch)
        query.addQueryItem(QStringLiteral("batch"),
                           *batch ? QStringLiteral("true") : QStringLiteral("false"));
    return query;
}

} // namespace Admin
} // namespace QtOpenAi
