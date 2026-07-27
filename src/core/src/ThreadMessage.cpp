// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ThreadMessage.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ThreadMessageData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    QString threadId;
    QString status;
    QJsonObject incompleteDetails;
    qint64 completedAt = 0;
    qint64 incompleteAt = 0;
    Role role = Role::User;
    QJsonArray content;
    QString assistantId;
    QString runId;
    QJsonArray attachments;
    QJsonObject metadata;
};

ThreadMessage::ThreadMessage()
    : d(new ThreadMessageData)
{ }

ThreadMessage::ThreadMessage(const ThreadMessage &other) = default;
ThreadMessage::ThreadMessage(ThreadMessage &&other) noexcept = default;
ThreadMessage &ThreadMessage::operator=(const ThreadMessage &other) = default;
ThreadMessage &ThreadMessage::operator=(ThreadMessage &&other) noexcept = default;
ThreadMessage::~ThreadMessage() = default;

QString ThreadMessage::id() const { return d->id; }
void ThreadMessage::setId(const QString &id) { d->id = id; }

QString ThreadMessage::object() const { return d->object; }
void ThreadMessage::setObject(const QString &object) { d->object = object; }

qint64 ThreadMessage::createdAt() const { return d->createdAt; }
void ThreadMessage::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString ThreadMessage::threadId() const { return d->threadId; }
void ThreadMessage::setThreadId(const QString &threadId) { d->threadId = threadId; }

QString ThreadMessage::status() const { return d->status; }
void ThreadMessage::setStatus(const QString &status) { d->status = status; }

QJsonObject ThreadMessage::incompleteDetails() const { return d->incompleteDetails; }
void ThreadMessage::setIncompleteDetails(const QJsonObject &incompleteDetails)
{
    d->incompleteDetails = incompleteDetails;
}

qint64 ThreadMessage::completedAt() const { return d->completedAt; }
void ThreadMessage::setCompletedAt(qint64 completedAt) { d->completedAt = completedAt; }

qint64 ThreadMessage::incompleteAt() const { return d->incompleteAt; }
void ThreadMessage::setIncompleteAt(qint64 incompleteAt) { d->incompleteAt = incompleteAt; }

Role ThreadMessage::role() const { return d->role; }
void ThreadMessage::setRole(Role role) { d->role = role; }

QJsonArray ThreadMessage::content() const { return d->content; }
void ThreadMessage::setContent(const QJsonArray &content) { d->content = content; }

QString ThreadMessage::text() const
{
    QString text;
    for (const QJsonValue &value : d->content) {
        const QJsonObject part = value.toObject();
        if (part.value(QStringLiteral("type")).toString() != QLatin1String("text"))
            continue;
        // The Assistants text part nests its value: {"text": {"value": ...}}.
        text += part.value(QStringLiteral("text"))
                        .toObject()
                        .value(QStringLiteral("value"))
                        .toString();
    }
    return text;
}

QString ThreadMessage::assistantId() const { return d->assistantId; }
void ThreadMessage::setAssistantId(const QString &assistantId) { d->assistantId = assistantId; }

QString ThreadMessage::runId() const { return d->runId; }
void ThreadMessage::setRunId(const QString &runId) { d->runId = runId; }

QJsonArray ThreadMessage::attachments() const { return d->attachments; }
void ThreadMessage::setAttachments(const QJsonArray &attachments) { d->attachments = attachments; }

QJsonObject ThreadMessage::metadata() const { return d->metadata; }
void ThreadMessage::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

QJsonObject ThreadMessage::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("thread_id"), d->threadId);
    detail::insertIfNotEmpty(json, QStringLiteral("status"), d->status);
    if (!d->incompleteDetails.isEmpty())
        json.insert(QStringLiteral("incomplete_details"), d->incompleteDetails);
    detail::insertIfNonZero(json, QStringLiteral("completed_at"), d->completedAt);
    detail::insertIfNonZero(json, QStringLiteral("incomplete_at"), d->incompleteAt);
    json.insert(QStringLiteral("role"), roleToString(d->role));
    if (!d->content.isEmpty())
        json.insert(QStringLiteral("content"), d->content);
    detail::insertIfNotEmpty(json, QStringLiteral("assistant_id"), d->assistantId);
    detail::insertIfNotEmpty(json, QStringLiteral("run_id"), d->runId);
    if (!d->attachments.isEmpty())
        json.insert(QStringLiteral("attachments"), d->attachments);
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    return json;
}

ThreadMessage ThreadMessage::fromJson(const QJsonObject &json)
{
    ThreadMessage message;
    message.d->id = detail::stringOr(json, QStringLiteral("id"));
    message.d->object = detail::stringOr(json, QStringLiteral("object"));
    message.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    message.d->threadId = detail::stringOr(json, QStringLiteral("thread_id"));
    message.d->status = detail::stringOr(json, QStringLiteral("status"));
    message.d->incompleteDetails = json.value(QStringLiteral("incomplete_details")).toObject();
    message.d->completedAt = detail::int64Or(json, QStringLiteral("completed_at"));
    message.d->incompleteAt = detail::int64Or(json, QStringLiteral("incomplete_at"));
    message.d->role = roleFromString(detail::stringOr(json, QStringLiteral("role")));
    message.d->content = json.value(QStringLiteral("content")).toArray();
    message.d->assistantId = detail::stringOr(json, QStringLiteral("assistant_id"));
    message.d->runId = detail::stringOr(json, QStringLiteral("run_id"));
    message.d->attachments = json.value(QStringLiteral("attachments")).toArray();
    message.d->metadata = json.value(QStringLiteral("metadata")).toObject();
    return message;
}

bool ThreadMessage::operator==(const ThreadMessage &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->threadId == other.d->threadId
           && d->status == other.d->status && d->incompleteDetails == other.d->incompleteDetails
           && d->completedAt == other.d->completedAt && d->incompleteAt == other.d->incompleteAt
           && d->role == other.d->role && d->content == other.d->content
           && d->assistantId == other.d->assistantId && d->runId == other.d->runId
           && d->attachments == other.d->attachments && d->metadata == other.d->metadata;
}

} // namespace Core
} // namespace QtOpenAi
