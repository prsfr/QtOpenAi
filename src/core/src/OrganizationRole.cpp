// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/OrganizationRole.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

QJsonObject AssignmentSource::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("principal_id"), principalId);
    detail::insertIfNotEmpty(json, QStringLiteral("principal_type"), principalType);
    return json;
}

AssignmentSource AssignmentSource::fromJson(const QJsonObject &json)
{
    AssignmentSource source;
    source.principalId = detail::stringOr(json, QStringLiteral("principal_id"));
    source.principalType = detail::stringOr(json, QStringLiteral("principal_type"));
    return source;
}

class OrganizationRoleData : public QSharedData
{
public:
    QString id;
    QString object;
    QString name;
    QString description;
    QStringList permissions;
    QString resourceType;
    bool predefinedRole = false;
    qint64 createdAt = 0;
    qint64 updatedAt = 0;
    QString createdBy;
    OrganizationUser createdByUser;
    QJsonObject metadata;
    QList<AssignmentSource> assignmentSources;
    bool deleted = false;
};

OrganizationRole::OrganizationRole()
    : d(new OrganizationRoleData)
{ }

OrganizationRole::OrganizationRole(const OrganizationRole &other) = default;
OrganizationRole::OrganizationRole(OrganizationRole &&other) noexcept = default;
OrganizationRole &OrganizationRole::operator=(const OrganizationRole &other) = default;
OrganizationRole &OrganizationRole::operator=(OrganizationRole &&other) noexcept = default;
OrganizationRole::~OrganizationRole() = default;

QString OrganizationRole::id() const { return d->id; }
void OrganizationRole::setId(const QString &id) { d->id = id; }

QString OrganizationRole::object() const { return d->object; }
void OrganizationRole::setObject(const QString &object) { d->object = object; }

QString OrganizationRole::name() const { return d->name; }
void OrganizationRole::setName(const QString &name) { d->name = name; }

QString OrganizationRole::description() const { return d->description; }
void OrganizationRole::setDescription(const QString &description) { d->description = description; }

QStringList OrganizationRole::permissions() const { return d->permissions; }
void OrganizationRole::setPermissions(const QStringList &permissions)
{
    d->permissions = permissions;
}

QString OrganizationRole::resourceType() const { return d->resourceType; }
void OrganizationRole::setResourceType(const QString &resourceType)
{
    d->resourceType = resourceType;
}

bool OrganizationRole::predefinedRole() const { return d->predefinedRole; }
void OrganizationRole::setPredefinedRole(bool predefinedRole)
{
    d->predefinedRole = predefinedRole;
}

qint64 OrganizationRole::createdAt() const { return d->createdAt; }
void OrganizationRole::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

qint64 OrganizationRole::updatedAt() const { return d->updatedAt; }
void OrganizationRole::setUpdatedAt(qint64 updatedAt) { d->updatedAt = updatedAt; }

QString OrganizationRole::createdBy() const { return d->createdBy; }
void OrganizationRole::setCreatedBy(const QString &createdBy) { d->createdBy = createdBy; }

OrganizationUser OrganizationRole::createdByUser() const { return d->createdByUser; }
void OrganizationRole::setCreatedByUser(const OrganizationUser &createdByUser)
{
    d->createdByUser = createdByUser;
}

QJsonObject OrganizationRole::metadata() const { return d->metadata; }
void OrganizationRole::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

QList<AssignmentSource> OrganizationRole::assignmentSources() const { return d->assignmentSources; }
void OrganizationRole::setAssignmentSources(const QList<AssignmentSource> &sources)
{
    d->assignmentSources = sources;
}

bool OrganizationRole::isDeleted() const { return d->deleted; }
void OrganizationRole::setDeleted(bool deleted) { d->deleted = deleted; }

QJsonObject OrganizationRole::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNotEmpty(json, QStringLiteral("description"), d->description);
    detail::insertIfNotEmpty(json, QStringLiteral("permissions"), d->permissions);
    detail::insertIfNotEmpty(json, QStringLiteral("resource_type"), d->resourceType);
    detail::insertIfTrue(json, QStringLiteral("predefined_role"), d->predefinedRole);

    // The provenance an assignment listing adds; absent for a role read out of
    // the catalogue, which is where most roles are read from.
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNonZero(json, QStringLiteral("updated_at"), d->updatedAt);
    detail::insertIfNotEmpty(json, QStringLiteral("created_by"), d->createdBy);
    const QJsonObject creator = d->createdByUser.toJson();
    if (!creator.isEmpty())
        json.insert(QStringLiteral("created_by_user_obj"), creator);
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    if (!d->assignmentSources.isEmpty()) {
        QJsonArray sources;
        for (const AssignmentSource &source : d->assignmentSources)
            sources.append(source.toJson());
        json.insert(QStringLiteral("assignment_sources"), sources);
    }

    detail::insertIfTrue(json, QStringLiteral("deleted"), d->deleted);
    return json;
}

OrganizationRole OrganizationRole::fromJson(const QJsonObject &json)
{
    OrganizationRole role;
    role.d->id = detail::stringOr(json, QStringLiteral("id"));
    role.d->object = detail::stringOr(json, QStringLiteral("object"));
    role.d->name = detail::stringOr(json, QStringLiteral("name"));
    role.d->description = detail::stringOr(json, QStringLiteral("description"));
    role.d->permissions = detail::stringListOr(json, QStringLiteral("permissions"));
    role.d->resourceType = detail::stringOr(json, QStringLiteral("resource_type"));
    role.d->predefinedRole = json.value(QStringLiteral("predefined_role")).toBool();
    role.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    role.d->updatedAt = detail::int64Or(json, QStringLiteral("updated_at"));
    role.d->createdBy = detail::stringOr(json, QStringLiteral("created_by"));
    role.d->createdByUser = OrganizationUser::fromJson(
            json.value(QStringLiteral("created_by_user_obj")).toObject());
    role.d->metadata = json.value(QStringLiteral("metadata")).toObject();

    const QJsonArray sources = json.value(QStringLiteral("assignment_sources")).toArray();
    role.d->assignmentSources.reserve(sources.size());
    for (const QJsonValue &source : sources)
        role.d->assignmentSources.append(AssignmentSource::fromJson(source.toObject()));

    role.d->deleted = json.value(QStringLiteral("deleted")).toBool();
    return role;
}

bool OrganizationRole::operator==(const OrganizationRole &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->name == other.d->name
           && d->description == other.d->description && d->permissions == other.d->permissions
           && d->resourceType == other.d->resourceType
           && d->predefinedRole == other.d->predefinedRole && d->createdAt == other.d->createdAt
           && d->updatedAt == other.d->updatedAt && d->createdBy == other.d->createdBy
           && d->createdByUser == other.d->createdByUser && d->metadata == other.d->metadata
           && d->assignmentSources == other.d->assignmentSources && d->deleted == other.d->deleted;
}

} // namespace Core
} // namespace QtOpenAi
