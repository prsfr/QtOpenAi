// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Thread.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ThreadData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    QJsonObject toolResources;
    QJsonObject metadata;
};

Thread::Thread()
    : d(new ThreadData)
{ }

Thread::Thread(const Thread &other) = default;
Thread::Thread(Thread &&other) noexcept = default;
Thread &Thread::operator=(const Thread &other) = default;
Thread &Thread::operator=(Thread &&other) noexcept = default;
Thread::~Thread() = default;

QString Thread::id() const { return d->id; }
void Thread::setId(const QString &id) { d->id = id; }

QString Thread::object() const { return d->object; }
void Thread::setObject(const QString &object) { d->object = object; }

qint64 Thread::createdAt() const { return d->createdAt; }
void Thread::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QJsonObject Thread::toolResources() const { return d->toolResources; }
void Thread::setToolResources(const QJsonObject &toolResources)
{
    d->toolResources = toolResources;
}

QJsonObject Thread::metadata() const { return d->metadata; }
void Thread::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

QJsonObject Thread::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    if (!d->toolResources.isEmpty())
        json.insert(QStringLiteral("tool_resources"), d->toolResources);
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    return json;
}

Thread Thread::fromJson(const QJsonObject &json)
{
    Thread thread;
    thread.d->id = detail::stringOr(json, QStringLiteral("id"));
    thread.d->object = detail::stringOr(json, QStringLiteral("object"));
    thread.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    thread.d->toolResources = json.value(QStringLiteral("tool_resources")).toObject();
    thread.d->metadata = json.value(QStringLiteral("metadata")).toObject();
    return thread;
}

bool Thread::operator==(const Thread &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->toolResources == other.d->toolResources
           && d->metadata == other.d->metadata;
}

} // namespace Core
} // namespace QtOpenAi
