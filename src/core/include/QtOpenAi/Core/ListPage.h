// SPDX-License-Identifier: MIT
#pragma once

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

// A single page of a cursor-paginated OpenAI `list` object.
//
// A lightweight, reusable value aggregate (like RetryPolicy/RateLimit) shared by
// every list endpoint. `T` must provide `QJsonObject toJson() const` and a
// `static T fromJson(const QJsonObject &)`. `firstId`/`lastId` are the cursors
// for the `after`/`before` query parameters, and `hasMore` signals a next page.
template <typename T>
struct ListPage
{
    QList<T> data;
    QString firstId;
    QString lastId;
    bool hasMore = false;

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
        if (!firstId.isEmpty())
            json.insert(QStringLiteral("first_id"), firstId);
        if (!lastId.isEmpty())
            json.insert(QStringLiteral("last_id"), lastId);
        json.insert(QStringLiteral("has_more"), hasMore);
        return json;
    }

    static ListPage fromJson(const QJsonObject &json)
    {
        ListPage page;
        const QJsonArray array = json.value(QStringLiteral("data")).toArray();
        for (const QJsonValue &value : array)
            page.data.append(T::fromJson(value.toObject()));
        page.firstId = json.value(QStringLiteral("first_id")).toString();
        page.lastId = json.value(QStringLiteral("last_id")).toString();
        page.hasMore = json.value(QStringLiteral("has_more")).toBool();
        return page;
    }

    bool operator==(const ListPage &other) const
    {
        return data == other.data && firstId == other.firstId && lastId == other.lastId
               && hasMore == other.hasMore;
    }
    bool operator!=(const ListPage &other) const { return !(*this == other); }
};

// The cursor for the page after this one, or empty when the walk is over.
//
// Found by argument-dependent lookup from Client::PageWalker, which is how one
// walker drives three page shapes that spell their cursor differently: this one
// advances by the last item's id, CursorPage by an opaque `next`, BucketPage by
// `next_page`. Folding `hasMore` in here means the walker never has to know
// which of the two signals -- the flag or an empty cursor -- a given endpoint
// actually uses.
template <typename T>
QString nextPageCursor(const ListPage<T> &page)
{
    return page.hasMore ? page.lastId : QString();
}

} // namespace Core
} // namespace QtOpenAi
