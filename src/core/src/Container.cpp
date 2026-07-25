// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Container.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

// --- Container -------------------------------------------------------------

class ContainerData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    QString name;
    QString status;
    QString expiresAfterAnchor;
    int expiresAfterMinutes = 0;
};

Container::Container()
    : d(new ContainerData)
{ }

Container::Container(const Container &other) = default;
Container::Container(Container &&other) noexcept = default;
Container &Container::operator=(const Container &other) = default;
Container &Container::operator=(Container &&other) noexcept = default;
Container::~Container() = default;

QString Container::id() const { return d->id; }
void Container::setId(const QString &id) { d->id = id; }

QString Container::object() const { return d->object; }
void Container::setObject(const QString &object) { d->object = object; }

qint64 Container::createdAt() const { return d->createdAt; }
void Container::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString Container::name() const { return d->name; }
void Container::setName(const QString &name) { d->name = name; }

QString Container::status() const { return d->status; }
void Container::setStatus(const QString &status) { d->status = status; }

QString Container::expiresAfterAnchor() const { return d->expiresAfterAnchor; }
int Container::expiresAfterMinutes() const { return d->expiresAfterMinutes; }

void Container::setExpiresAfter(const QString &anchor, int minutes)
{
    d->expiresAfterAnchor = anchor;
    d->expiresAfterMinutes = minutes;
}

QJsonObject Container::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNotEmpty(json, QStringLiteral("status"), d->status);
    if (d->expiresAfterMinutes > 0) {
        QJsonObject expiresAfter;
        detail::insertIfNotEmpty(expiresAfter, QStringLiteral("anchor"), d->expiresAfterAnchor);
        expiresAfter.insert(QStringLiteral("minutes"), d->expiresAfterMinutes);
        json.insert(QStringLiteral("expires_after"), expiresAfter);
    }
    return json;
}

Container Container::fromJson(const QJsonObject &json)
{
    Container container;
    container.d->id = detail::stringOr(json, QStringLiteral("id"));
    container.d->object = detail::stringOr(json, QStringLiteral("object"));
    container.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    container.d->name = detail::stringOr(json, QStringLiteral("name"));
    container.d->status = detail::stringOr(json, QStringLiteral("status"));
    const QJsonValue expiresAfter = json.value(QStringLiteral("expires_after"));
    if (expiresAfter.isObject()) {
        const QJsonObject object = expiresAfter.toObject();
        container.d->expiresAfterAnchor = detail::stringOr(object, QStringLiteral("anchor"));
        container.d->expiresAfterMinutes = object.value(QStringLiteral("minutes")).toInt();
    }
    return container;
}

bool Container::operator==(const Container &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->name == other.d->name
           && d->status == other.d->status && d->expiresAfterAnchor == other.d->expiresAfterAnchor
           && d->expiresAfterMinutes == other.d->expiresAfterMinutes;
}

// --- ContainerFile ---------------------------------------------------------

class ContainerFileData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    qint64 bytes = 0;
    QString containerId;
    QString path;
    QString source;
};

ContainerFile::ContainerFile()
    : d(new ContainerFileData)
{ }

ContainerFile::ContainerFile(const ContainerFile &other) = default;
ContainerFile::ContainerFile(ContainerFile &&other) noexcept = default;
ContainerFile &ContainerFile::operator=(const ContainerFile &other) = default;
ContainerFile &ContainerFile::operator=(ContainerFile &&other) noexcept = default;
ContainerFile::~ContainerFile() = default;

QString ContainerFile::id() const { return d->id; }
void ContainerFile::setId(const QString &id) { d->id = id; }

QString ContainerFile::object() const { return d->object; }
void ContainerFile::setObject(const QString &object) { d->object = object; }

qint64 ContainerFile::createdAt() const { return d->createdAt; }
void ContainerFile::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

qint64 ContainerFile::bytes() const { return d->bytes; }
void ContainerFile::setBytes(qint64 bytes) { d->bytes = bytes; }

QString ContainerFile::containerId() const { return d->containerId; }
void ContainerFile::setContainerId(const QString &containerId) { d->containerId = containerId; }

QString ContainerFile::path() const { return d->path; }
void ContainerFile::setPath(const QString &path) { d->path = path; }

QString ContainerFile::source() const { return d->source; }
void ContainerFile::setSource(const QString &source) { d->source = source; }

QJsonObject ContainerFile::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNonZero(json, QStringLiteral("bytes"), d->bytes);
    detail::insertIfNotEmpty(json, QStringLiteral("container_id"), d->containerId);
    detail::insertIfNotEmpty(json, QStringLiteral("path"), d->path);
    detail::insertIfNotEmpty(json, QStringLiteral("source"), d->source);
    return json;
}

ContainerFile ContainerFile::fromJson(const QJsonObject &json)
{
    ContainerFile file;
    file.d->id = detail::stringOr(json, QStringLiteral("id"));
    file.d->object = detail::stringOr(json, QStringLiteral("object"));
    file.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    file.d->bytes = detail::int64Or(json, QStringLiteral("bytes"));
    file.d->containerId = detail::stringOr(json, QStringLiteral("container_id"));
    file.d->path = detail::stringOr(json, QStringLiteral("path"));
    file.d->source = detail::stringOr(json, QStringLiteral("source"));
    return file;
}

bool ContainerFile::operator==(const ContainerFile &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->bytes == other.d->bytes
           && d->containerId == other.d->containerId && d->path == other.d->path
           && d->source == other.d->source;
}

} // namespace Core
} // namespace QtOpenAi
