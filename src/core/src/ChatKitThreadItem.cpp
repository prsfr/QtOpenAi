// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ChatKitThreadItem.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

namespace {

// The fields this type models. Everything else belongs to one variant of the
// union and is carried in raw(), so the split is spelled once rather than in
// both directions of the serialisation.
constexpr QLatin1String kEnvelopeKeys[] = {
        QLatin1String("id"),        QLatin1String("object"), QLatin1String("created_at"),
        QLatin1String("thread_id"), QLatin1String("type"),   QLatin1String("content"),
};

bool isEnvelopeKey(const QString &key)
{
    for (QLatin1String modelled : kEnvelopeKeys) {
        if (key == modelled)
            return true;
    }
    return false;
}

} // namespace

class ChatKitThreadItemData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    QString threadId;
    QString type;
    QJsonArray content;
    QJsonObject raw;
};

ChatKitThreadItem::ChatKitThreadItem()
    : d(new ChatKitThreadItemData)
{ }

ChatKitThreadItem::ChatKitThreadItem(const ChatKitThreadItem &other) = default;
ChatKitThreadItem::ChatKitThreadItem(ChatKitThreadItem &&other) noexcept = default;
ChatKitThreadItem &ChatKitThreadItem::operator=(const ChatKitThreadItem &other) = default;
ChatKitThreadItem &ChatKitThreadItem::operator=(ChatKitThreadItem &&other) noexcept = default;
ChatKitThreadItem::~ChatKitThreadItem() = default;

QString ChatKitThreadItem::id() const { return d->id; }
void ChatKitThreadItem::setId(const QString &id) { d->id = id; }

QString ChatKitThreadItem::object() const { return d->object; }
void ChatKitThreadItem::setObject(const QString &object) { d->object = object; }

qint64 ChatKitThreadItem::createdAt() const { return d->createdAt; }
void ChatKitThreadItem::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString ChatKitThreadItem::threadId() const { return d->threadId; }
void ChatKitThreadItem::setThreadId(const QString &threadId) { d->threadId = threadId; }

QString ChatKitThreadItem::type() const { return d->type; }
void ChatKitThreadItem::setType(const QString &type) { d->type = type; }

QJsonArray ChatKitThreadItem::content() const { return d->content; }
void ChatKitThreadItem::setContent(const QJsonArray &content) { d->content = content; }

QJsonObject ChatKitThreadItem::raw() const { return d->raw; }
void ChatKitThreadItem::setRaw(const QJsonObject &raw) { d->raw = raw; }

QString ChatKitThreadItem::text() const
{
    QString text;
    for (const QJsonValue &value : d->content)
        text += detail::stringOr(value.toObject(), QStringLiteral("text"));
    return text;
}

QJsonObject ChatKitThreadItem::toJson() const
{
    QJsonObject json = d->raw;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("thread_id"), d->threadId);
    detail::insertIfNotEmpty(json, QStringLiteral("type"), d->type);
    if (!d->content.isEmpty())
        json.insert(QStringLiteral("content"), d->content);
    return json;
}

ChatKitThreadItem ChatKitThreadItem::fromJson(const QJsonObject &json)
{
    ChatKitThreadItem item;
    item.d->id = detail::stringOr(json, QStringLiteral("id"));
    item.d->object = detail::stringOr(json, QStringLiteral("object"));
    item.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    item.d->threadId = detail::stringOr(json, QStringLiteral("thread_id"));
    item.d->type = detail::stringOr(json, QStringLiteral("type"));
    item.d->content = json.value(QStringLiteral("content")).toArray();
    for (auto it = json.constBegin(); it != json.constEnd(); ++it) {
        if (!isEnvelopeKey(it.key()))
            item.d->raw.insert(it.key(), it.value());
    }
    return item;
}

bool ChatKitThreadItem::operator==(const ChatKitThreadItem &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->threadId == other.d->threadId
           && d->type == other.d->type && d->content == other.d->content && d->raw == other.d->raw;
}

} // namespace Core
} // namespace QtOpenAi
