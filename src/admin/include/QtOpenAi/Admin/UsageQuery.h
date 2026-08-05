// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QUrlQuery>

#include <optional>

namespace QtOpenAi {
namespace Admin {

// The query parameters of an administration report: the ten usage endpoints and
// /organization/costs all take the same time window, bucket width, grouping and
// filters.
//
// One type for all eleven, rather than eleven signatures spelling out the same
// six parameters — this is the part of the surface most likely to be silently
// wrong, since a query the server does not understand comes back as a perfectly
// valid report of the wrong thing.
//
//     Admin::UsageQuery query;
//     query.startTime = QDateTime::currentSecsSinceEpoch() - 7 * 24 * 3600;
//     query.bucketWidth = QStringLiteral("1d");
//     query.groupBy = {QStringLiteral("model")};
//
// Not every endpoint honours every field — `userIds` and `models` mean nothing
// to /organization/costs, and `batch` only to completions. Each is documented
// below with where it applies; sending one that does not apply is the caller's
// error to make, and keeping eleven near-identical types to prevent it would
// cost more than it saves.
struct QTOPENAI_ADMIN_EXPORT UsageQuery
{
    // **Required by the API.** Unix seconds, inclusive. A report with no start
    // is refused by the server rather than defaulted, so it is left at 0 here
    // rather than guessed at.
    qint64 startTime = 0;

    // Unix seconds, exclusive; 0 means "up to now".
    qint64 endTime = 0;

    // "1m", "1h" or "1d"; empty leaves the server default ("1d"). A string
    // rather than an enum for the same reason Project::status is: a width this
    // build has never heard of has to reach the server, not fail here.
    QString bucketWidth;

    // What to split each bucket by: "project_id", "user_id", "api_key_id",
    // "model", "batch" for usage; "project_id", "line_item" for costs. Ungrouped
    // (the default) each bucket holds a single total row.
    QStringList groupBy;

    // Number of buckets to return (-1 leaves the server default).
    int limit = -1;

    // The `next_page` cursor of a previous BucketPage. Not the after/before ids
    // of ListParams: the report endpoints paginate by opaque page token.
    QString page;

    // Filters. Each restricts the report to the listed ids; empty means all.
    // `projectIds` applies everywhere, the rest only to /organization/usage/*.
    QStringList projectIds;
    QStringList userIds;
    QStringList apiKeyIds;
    QStringList models;

    // Completions only: true for batch usage, false for interactive usage,
    // unset for both. Tri-state for the same reason UsageResult::batch is.
    std::optional<bool> batch;

    QUrlQuery toQuery() const;
};

} // namespace Admin
} // namespace QtOpenAi
