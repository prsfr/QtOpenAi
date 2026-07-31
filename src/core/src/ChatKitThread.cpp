// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ChatKitThread.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ChatKitThreadData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    QString title;
    ChatKitThreadStatus status = ChatKitThreadStatus::Active;
    QString statusReason;
    QString user;
};

ChatKitThread::ChatKitThread()
    : d(new ChatKitThreadData)
{ }

ChatKitThread::ChatKitThread(const ChatKitThread &other) = default;
ChatKitThread::ChatKitThread(ChatKitThread &&other) noexcept = default;
ChatKitThread &ChatKitThread::operator=(const ChatKitThread &other) = default;
ChatKitThread &ChatKitThread::operator=(ChatKitThread &&other) noexcept = default;
ChatKitThread::~ChatKitThread() = default;

QString ChatKitThread::id() const { return d->id; }
void ChatKitThread::setId(const QString &id) { d->id = id; }

QString ChatKitThread::object() const { return d->object; }
void ChatKitThread::setObject(const QString &object) { d->object = object; }

qint64 ChatKitThread::createdAt() const { return d->createdAt; }
void ChatKitThread::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString ChatKitThread::title() const { return d->title; }
void ChatKitThread::setTitle(const QString &title) { d->title = title; }

ChatKitThreadStatus ChatKitThread::status() const { return d->status; }
void ChatKitThread::setStatus(ChatKitThreadStatus status) { d->status = status; }

QString ChatKitThread::statusReason() const { return d->statusReason; }
void ChatKitThread::setStatusReason(const QString &statusReason) { d->statusReason = statusReason; }

QString ChatKitThread::user() const { return d->user; }
void ChatKitThread::setUser(const QString &user) { d->user = user; }

QJsonObject ChatKitThread::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("title"), d->title);
    QJsonObject status;
    status.insert(QStringLiteral("type"), chatKitThreadStatusToString(d->status));
    detail::insertIfNotEmpty(status, QStringLiteral("reason"), d->statusReason);
    json.insert(QStringLiteral("status"), status);
    detail::insertIfNotEmpty(json, QStringLiteral("user"), d->user);
    return json;
}

ChatKitThread ChatKitThread::fromJson(const QJsonObject &json)
{
    ChatKitThread thread;
    thread.d->id = detail::stringOr(json, QStringLiteral("id"));
    thread.d->object = detail::stringOr(json, QStringLiteral("object"));
    thread.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    thread.d->title = detail::stringOr(json, QStringLiteral("title"));
    const QJsonObject status = json.value(QStringLiteral("status")).toObject();
    thread.d->status
            = chatKitThreadStatusFromString(detail::stringOr(status, QStringLiteral("type")));
    thread.d->statusReason = detail::stringOr(status, QStringLiteral("reason"));
    thread.d->user = detail::stringOr(json, QStringLiteral("user"));
    return thread;
}

bool ChatKitThread::operator==(const ChatKitThread &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->title == other.d->title
           && d->status == other.d->status && d->statusReason == other.d->statusReason
           && d->user == other.d->user;
}

} // namespace Core
} // namespace QtOpenAi
