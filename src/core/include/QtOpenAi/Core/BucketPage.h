// SPDX-License-Identifier: MIT
#pragma once

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

// One time bucket of an administration report: everything that happened between
// `startTime` and `endTime`, already aggregated by the server.
//
// A lightweight, reusable value aggregate (like ListPage) shared by the eleven
// endpoints under /organization/usage and /organization/costs. `T` must provide
// `QJsonObject toJson() const` and a `static T fromJson(const QJsonObject &)`.
//
// The bucket is a template rather than one struct per report because the bucket
// is the part those endpoints genuinely share: the envelope — a time range and a
// list of rows — is identical everywhere, and only the row differs. Writing it
// once means a bucket cannot decode differently for costs than it does for
// completions.
template <typename T>
struct Bucket
{
    // Unix timestamps in seconds, inclusive start and exclusive end.
    qint64 startTime = 0;
    qint64 endTime = 0;

    // The rows in this bucket: one per group when the query asked to group, and
    // otherwise a single total. Empty when nothing happened in the interval —
    // the server still sends the bucket, which is what makes a report plottable
    // without filling gaps by hand.
    QList<T> results;

    bool isEmpty() const { return results.isEmpty(); }
    int size() const { return results.size(); }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json.insert(QStringLiteral("object"), QStringLiteral("bucket"));
        json.insert(QStringLiteral("start_time"), startTime);
        json.insert(QStringLiteral("end_time"), endTime);
        QJsonArray array;
        for (const T &result : results)
            array.append(result.toJson());
        json.insert(QStringLiteral("results"), array);
        return json;
    }

    static Bucket fromJson(const QJsonObject &json)
    {
        Bucket bucket;
        // Through QVariant rather than toDouble(): a unix timestamp fits a
        // double today, but the rest of this library reads 64-bit integers this
        // way and one exception is how the odd one out gets it wrong.
        bucket.startTime = json.value(QStringLiteral("start_time")).toVariant().toLongLong();
        bucket.endTime = json.value(QStringLiteral("end_time")).toVariant().toLongLong();
        const QJsonArray array = json.value(QStringLiteral("results")).toArray();
        bucket.results.reserve(array.size());
        for (const QJsonValue &value : array)
            bucket.results.append(T::fromJson(value.toObject()));
        return bucket;
    }

    bool operator==(const Bucket &other) const
    {
        return startTime == other.startTime && endTime == other.endTime && results == other.results;
    }
    bool operator!=(const Bucket &other) const { return !(*this == other); }
};

// A `page` of time buckets, the response shape of every administration report.
//
// Paginated by an opaque `next_page` cursor rather than by the `after`/`before`
// item ids of ListPage, which is why this is a separate type and not a reuse of
// that one: pass `nextPage` back as UsageQuery::page to fetch the rest.
template <typename T>
struct BucketPage
{
    QList<Bucket<T>> data;
    bool hasMore = false;
    QString nextPage;

    bool isEmpty() const { return data.isEmpty(); }
    int size() const { return data.size(); }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json.insert(QStringLiteral("object"), QStringLiteral("page"));
        QJsonArray array;
        for (const Bucket<T> &bucket : data)
            array.append(bucket.toJson());
        json.insert(QStringLiteral("data"), array);
        json.insert(QStringLiteral("has_more"), hasMore);
        // Absent rather than null on the last page: the two mean the same thing
        // to a reader, and one spelling is enough.
        if (!nextPage.isEmpty())
            json.insert(QStringLiteral("next_page"), nextPage);
        return json;
    }

    static BucketPage fromJson(const QJsonObject &json)
    {
        BucketPage page;
        const QJsonArray array = json.value(QStringLiteral("data")).toArray();
        page.data.reserve(array.size());
        for (const QJsonValue &value : array)
            page.data.append(Bucket<T>::fromJson(value.toObject()));
        page.hasMore = json.value(QStringLiteral("has_more")).toBool();
        page.nextPage = json.value(QStringLiteral("next_page")).toString();
        return page;
    }

    bool operator==(const BucketPage &other) const
    {
        return data == other.data && hasMore == other.hasMore && nextPage == other.nextPage;
    }
    bool operator!=(const BucketPage &other) const { return !(*this == other); }
};

// See ListPage's overload. This one advances by `next_page`, and the query
// parameter it goes back into is `page` rather than `after` -- see
// Admin::UsageQuery's applyPageCursor().
template <typename T>
QString nextPageCursor(const BucketPage<T> &page)
{
    return page.hasMore ? page.nextPage : QString();
}

} // namespace Core
} // namespace QtOpenAi
