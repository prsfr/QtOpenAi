// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ConversationItemList.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ConversationItemListData : public QSharedData
{
public:
    QList<ResponseOutputItem> items;
    QString firstId;
    QString lastId;
    bool hasMore = false;
};

ConversationItemList::ConversationItemList()
    : d(new ConversationItemListData)
{ }

ConversationItemList::ConversationItemList(const ConversationItemList &other) = default;
ConversationItemList::ConversationItemList(ConversationItemList &&other) noexcept = default;
ConversationItemList &ConversationItemList::operator=(const ConversationItemList &other) = default;
ConversationItemList &ConversationItemList::operator=(ConversationItemList &&other) noexcept
        = default;
ConversationItemList::~ConversationItemList() = default;

QList<ResponseOutputItem> ConversationItemList::items() const { return d->items; }
void ConversationItemList::setItems(const QList<ResponseOutputItem> &items) { d->items = items; }

QString ConversationItemList::firstId() const { return d->firstId; }
void ConversationItemList::setFirstId(const QString &id) { d->firstId = id; }

QString ConversationItemList::lastId() const { return d->lastId; }
void ConversationItemList::setLastId(const QString &id) { d->lastId = id; }

bool ConversationItemList::hasMore() const { return d->hasMore; }
void ConversationItemList::setHasMore(bool hasMore) { d->hasMore = hasMore; }

QJsonObject ConversationItemList::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("object"), QStringLiteral("list"));
    QJsonArray data;
    for (const ResponseOutputItem &item : d->items)
        data.append(item.toJson());
    json.insert(QStringLiteral("data"), data);
    detail::insertIfNotEmpty(json, QStringLiteral("first_id"), d->firstId);
    detail::insertIfNotEmpty(json, QStringLiteral("last_id"), d->lastId);
    json.insert(QStringLiteral("has_more"), d->hasMore);
    return json;
}

ConversationItemList ConversationItemList::fromJson(const QJsonObject &json)
{
    ConversationItemList list;
    const QJsonArray data = json.value(QStringLiteral("data")).toArray();
    for (const QJsonValue &value : data)
        list.d->items.append(ResponseOutputItem::fromJson(value.toObject()));
    list.d->firstId = detail::stringOr(json, QStringLiteral("first_id"));
    list.d->lastId = detail::stringOr(json, QStringLiteral("last_id"));
    list.d->hasMore = json.value(QStringLiteral("has_more")).toBool();
    return list;
}

bool ConversationItemList::operator==(const ConversationItemList &other) const
{
    return d->items == other.d->items && d->firstId == other.d->firstId
           && d->lastId == other.d->lastId && d->hasMore == other.d->hasMore;
}

} // namespace Core
} // namespace QtOpenAi
