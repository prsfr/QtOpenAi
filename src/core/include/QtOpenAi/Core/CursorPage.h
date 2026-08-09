// SPDX-License-Identifier: MIT
#pragma once

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

// One page of a `list` that paginates by an opaque `next` cursor.
//
// A lightweight, reusable value aggregate (like ListPage and BucketPage) shared
// by the roles and groups endpoints. `T` must provide `QJsonObject toJson()
// const` and a `static T fromJson(const QJsonObject &)`.
//
// **It is not ListPage, because the envelope is genuinely a different one.**
// ListPage carries `first_id`/`last_id`: the ids of the first and last items on
// the page, which the caller passes back as `after`/`before` to walk in either
// direction. The roles and groups endpoints send neither. They send a single
// `next` — a server-minted cursor that is not an item id, that only goes
// forwards, and that is `null` on the last page. Decoding one shape into the
// other would have meant a `firstId` that is always empty and a `lastId` that
// silently means something else than it does everywhere else in this library.
//
// The cursor goes back as `after`:
//
//     Client::ListParams params;
//     params.after = page.next;      // empty on the last page
template <typename T>
struct CursorPage
{
    QList<T> data;
    bool hasMore = false;

    // The cursor for the next page, or empty when there is none. The API sends
    // `null` there, which reads back as an empty string: both mean "this was the
    // last page", and `hasMore` is the flag to branch on.
    QString next;

    bool isEmpty() const { return data.isEmpty(); }
    int size() const { return data.size(); }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json.insert(QStringLiteral("object"), QStringLiteral("list"));
        QJsonArray array;
        for (const T &item : data)
            array.append(item.toJson());
        json.insert(QStringLiteral("data"), array);
        json.insert(QStringLiteral("has_more"), hasMore);
        // Absent rather than written as null on the last page, as BucketPage
        // leaves out its exhausted `next_page`: one spelling is enough.
        if (!next.isEmpty())
            json.insert(QStringLiteral("next"), next);
        return json;
    }

    static CursorPage fromJson(const QJsonObject &json)
    {
        CursorPage page;
        const QJsonArray array = json.value(QStringLiteral("data")).toArray();
        page.data.reserve(array.size());
        for (const QJsonValue &value : array)
            page.data.append(T::fromJson(value.toObject()));
        page.hasMore = json.value(QStringLiteral("has_more")).toBool();
        page.next = json.value(QStringLiteral("next")).toString();
        return page;
    }

    bool operator==(const CursorPage &other) const
    {
        return data == other.data && hasMore == other.hasMore && next == other.next;
    }
    bool operator!=(const CursorPage &other) const { return !(*this == other); }
};

} // namespace Core
} // namespace QtOpenAi
