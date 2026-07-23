// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Conversation.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ConversationData : public QSharedData
{
public:
    QString id;
    QString object = QStringLiteral("conversation");
    qint64 createdAt = 0;
    QJsonObject metadata;
};

Conversation::Conversation()
    : d(new ConversationData)
{ }

Conversation::Conversation(const Conversation &other) = default;
Conversation::Conversation(Conversation &&other) noexcept = default;
Conversation &Conversation::operator=(const Conversation &other) = default;
Conversation &Conversation::operator=(Conversation &&other) noexcept = default;
Conversation::~Conversation() = default;

QString Conversation::id() const { return d->id; }
void Conversation::setId(const QString &id) { d->id = id; }

QString Conversation::object() const { return d->object; }
void Conversation::setObject(const QString &object) { d->object = object; }

qint64 Conversation::createdAt() const { return d->createdAt; }
void Conversation::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QJsonObject Conversation::metadata() const { return d->metadata; }
void Conversation::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

QJsonObject Conversation::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("id"), d->id);
    json.insert(QStringLiteral("object"), d->object);
    json.insert(QStringLiteral("created_at"), d->createdAt);
    json.insert(QStringLiteral("metadata"), d->metadata);
    return json;
}

Conversation Conversation::fromJson(const QJsonObject &json)
{
    Conversation conversation;
    conversation.d->id = detail::stringOr(json, QStringLiteral("id"));
    conversation.d->object
            = detail::stringOr(json, QStringLiteral("object"), QStringLiteral("conversation"));
    conversation.d->createdAt
            = static_cast<qint64>(json.value(QStringLiteral("created_at")).toDouble());
    conversation.d->metadata = json.value(QStringLiteral("metadata")).toObject();
    return conversation;
}

bool Conversation::operator==(const Conversation &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->metadata == other.d->metadata;
}

} // namespace Core
} // namespace QtOpenAi
