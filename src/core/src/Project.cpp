// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Project.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ProjectData : public QSharedData
{
public:
    QString id;
    QString object;
    QString name;
    qint64 createdAt = 0;
    qint64 archivedAt = 0;
    QString status;
};

Project::Project()
    : d(new ProjectData)
{ }

Project::Project(const Project &other) = default;
Project::Project(Project &&other) noexcept = default;
Project &Project::operator=(const Project &other) = default;
Project &Project::operator=(Project &&other) noexcept = default;
Project::~Project() = default;

QString Project::id() const { return d->id; }
void Project::setId(const QString &id) { d->id = id; }

QString Project::object() const { return d->object; }
void Project::setObject(const QString &object) { d->object = object; }

QString Project::name() const { return d->name; }
void Project::setName(const QString &name) { d->name = name; }

qint64 Project::createdAt() const { return d->createdAt; }
void Project::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

qint64 Project::archivedAt() const { return d->archivedAt; }
void Project::setArchivedAt(qint64 archivedAt) { d->archivedAt = archivedAt; }

QString Project::status() const { return d->status; }
void Project::setStatus(const QString &status) { d->status = status; }

QJsonObject Project::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    // Left out entirely rather than written as null: an absent archived_at and
    // a null one both mean "still active", and one spelling is enough.
    detail::insertIfNonZero(json, QStringLiteral("archived_at"), d->archivedAt);
    detail::insertIfNotEmpty(json, QStringLiteral("status"), d->status);
    return json;
}

Project Project::fromJson(const QJsonObject &json)
{
    Project project;
    project.d->id = detail::stringOr(json, QStringLiteral("id"));
    project.d->object = detail::stringOr(json, QStringLiteral("object"));
    project.d->name = detail::stringOr(json, QStringLiteral("name"));
    project.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    project.d->archivedAt = detail::int64Or(json, QStringLiteral("archived_at"));
    project.d->status = detail::stringOr(json, QStringLiteral("status"));
    return project;
}

bool Project::operator==(const Project &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->name == other.d->name
           && d->createdAt == other.d->createdAt && d->archivedAt == other.d->archivedAt
           && d->status == other.d->status;
}

} // namespace Core
} // namespace QtOpenAi
