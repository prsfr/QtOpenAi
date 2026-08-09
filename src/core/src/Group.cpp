// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Group.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class GroupData : public QSharedData
{
public:
    QString id;
    QString object;
    QString name;
    qint64 createdAt = 0;
    bool scimManaged = false;
    QString groupType;
    bool deleted = false;
};

Group::Group()
    : d(new GroupData)
{ }

Group::Group(const Group &other) = default;
Group::Group(Group &&other) noexcept = default;
Group &Group::operator=(const Group &other) = default;
Group &Group::operator=(Group &&other) noexcept = default;
Group::~Group() = default;

QString Group::id() const { return d->id; }
void Group::setId(const QString &id) { d->id = id; }

QString Group::object() const { return d->object; }
void Group::setObject(const QString &object) { d->object = object; }

QString Group::name() const { return d->name; }
void Group::setName(const QString &name) { d->name = name; }

qint64 Group::createdAt() const { return d->createdAt; }
void Group::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

bool Group::isScimManaged() const { return d->scimManaged; }
void Group::setScimManaged(bool scimManaged) { d->scimManaged = scimManaged; }

QString Group::groupType() const { return d->groupType; }
void Group::setGroupType(const QString &groupType) { d->groupType = groupType; }

bool Group::isDeleted() const { return d->deleted; }
void Group::setDeleted(bool deleted) { d->deleted = deleted; }

QJsonObject Group::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    // The spelling the groups endpoints use. fromJson() accepts the shorter
    // `scim_managed` an embedded group arrives with, but one spelling goes out.
    detail::insertIfTrue(json, QStringLiteral("is_scim_managed"), d->scimManaged);
    detail::insertIfNotEmpty(json, QStringLiteral("group_type"), d->groupType);
    detail::insertIfTrue(json, QStringLiteral("deleted"), d->deleted);
    return json;
}

Group Group::fromJson(const QJsonObject &json)
{
    Group group;
    group.d->id = detail::stringOr(json, QStringLiteral("id"));
    group.d->object = detail::stringOr(json, QStringLiteral("object"));
    group.d->name = detail::stringOr(json, QStringLiteral("name"));
    group.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    // Both spellings: the groups endpoints send `is_scim_managed`, and the group
    // embedded in a role assignment sends `scim_managed`. Reading only one would
    // report a synchronised group as editable in exactly one of the two places.
    group.d->scimManaged = json.value(QStringLiteral("is_scim_managed"))
                                   .toBool(json.value(QStringLiteral("scim_managed")).toBool());
    group.d->groupType = detail::stringOr(json, QStringLiteral("group_type"));
    group.d->deleted = json.value(QStringLiteral("deleted")).toBool();
    return group;
}

bool Group::operator==(const Group &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->name == other.d->name
           && d->createdAt == other.d->createdAt && d->scimManaged == other.d->scimManaged
           && d->groupType == other.d->groupType && d->deleted == other.d->deleted;
}

class GroupMemberData : public QSharedData
{
public:
    QString id;
    QString name;
    QString email;
    QString picture;
    bool serviceAccount = false;
    QString userType;
};

GroupMember::GroupMember()
    : d(new GroupMemberData)
{ }

GroupMember::GroupMember(const GroupMember &other) = default;
GroupMember::GroupMember(GroupMember &&other) noexcept = default;
GroupMember &GroupMember::operator=(const GroupMember &other) = default;
GroupMember &GroupMember::operator=(GroupMember &&other) noexcept = default;
GroupMember::~GroupMember() = default;

QString GroupMember::id() const { return d->id; }
void GroupMember::setId(const QString &id) { d->id = id; }

QString GroupMember::name() const { return d->name; }
void GroupMember::setName(const QString &name) { d->name = name; }

QString GroupMember::email() const { return d->email; }
void GroupMember::setEmail(const QString &email) { d->email = email; }

QString GroupMember::picture() const { return d->picture; }
void GroupMember::setPicture(const QString &picture) { d->picture = picture; }

bool GroupMember::isServiceAccount() const { return d->serviceAccount; }
void GroupMember::setServiceAccount(bool serviceAccount) { d->serviceAccount = serviceAccount; }

QString GroupMember::userType() const { return d->userType; }
void GroupMember::setUserType(const QString &userType) { d->userType = userType; }

QJsonObject GroupMember::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNotEmpty(json, QStringLiteral("email"), d->email);
    detail::insertIfNotEmpty(json, QStringLiteral("picture"), d->picture);
    detail::insertIfTrue(json, QStringLiteral("is_service_account"), d->serviceAccount);
    detail::insertIfNotEmpty(json, QStringLiteral("user_type"), d->userType);
    return json;
}

GroupMember GroupMember::fromJson(const QJsonObject &json)
{
    GroupMember member;
    member.d->id = detail::stringOr(json, QStringLiteral("id"));
    member.d->name = detail::stringOr(json, QStringLiteral("name"));
    member.d->email = detail::stringOr(json, QStringLiteral("email"));
    member.d->picture = detail::stringOr(json, QStringLiteral("picture"));
    member.d->serviceAccount = json.value(QStringLiteral("is_service_account")).toBool();
    member.d->userType = detail::stringOr(json, QStringLiteral("user_type"));
    return member;
}

bool GroupMember::operator==(const GroupMember &other) const
{
    return d->id == other.d->id && d->name == other.d->name && d->email == other.d->email
           && d->picture == other.d->picture && d->serviceAccount == other.d->serviceAccount
           && d->userType == other.d->userType;
}

class GroupMembershipData : public QSharedData
{
public:
    QString object;
    QString groupId;
    QString userId;
    bool deleted = false;
};

GroupMembership::GroupMembership()
    : d(new GroupMembershipData)
{ }

GroupMembership::GroupMembership(const GroupMembership &other) = default;
GroupMembership::GroupMembership(GroupMembership &&other) noexcept = default;
GroupMembership &GroupMembership::operator=(const GroupMembership &other) = default;
GroupMembership &GroupMembership::operator=(GroupMembership &&other) noexcept = default;
GroupMembership::~GroupMembership() = default;

QString GroupMembership::object() const { return d->object; }
void GroupMembership::setObject(const QString &object) { d->object = object; }

QString GroupMembership::groupId() const { return d->groupId; }
void GroupMembership::setGroupId(const QString &groupId) { d->groupId = groupId; }

QString GroupMembership::userId() const { return d->userId; }
void GroupMembership::setUserId(const QString &userId) { d->userId = userId; }

bool GroupMembership::isDeleted() const { return d->deleted; }
void GroupMembership::setDeleted(bool deleted) { d->deleted = deleted; }

QJsonObject GroupMembership::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("group_id"), d->groupId);
    detail::insertIfNotEmpty(json, QStringLiteral("user_id"), d->userId);
    detail::insertIfTrue(json, QStringLiteral("deleted"), d->deleted);
    return json;
}

GroupMembership GroupMembership::fromJson(const QJsonObject &json)
{
    GroupMembership membership;
    membership.d->object = detail::stringOr(json, QStringLiteral("object"));
    membership.d->groupId = detail::stringOr(json, QStringLiteral("group_id"));
    membership.d->userId = detail::stringOr(json, QStringLiteral("user_id"));
    membership.d->deleted = json.value(QStringLiteral("deleted")).toBool();
    return membership;
}

bool GroupMembership::operator==(const GroupMembership &other) const
{
    return d->object == other.d->object && d->groupId == other.d->groupId
           && d->userId == other.d->userId && d->deleted == other.d->deleted;
}

class ProjectGroupData : public QSharedData
{
public:
    QString object;
    QString projectId;
    QString groupId;
    QString groupName;
    QString groupType;
    qint64 createdAt = 0;
    bool deleted = false;
};

ProjectGroup::ProjectGroup()
    : d(new ProjectGroupData)
{ }

ProjectGroup::ProjectGroup(const ProjectGroup &other) = default;
ProjectGroup::ProjectGroup(ProjectGroup &&other) noexcept = default;
ProjectGroup &ProjectGroup::operator=(const ProjectGroup &other) = default;
ProjectGroup &ProjectGroup::operator=(ProjectGroup &&other) noexcept = default;
ProjectGroup::~ProjectGroup() = default;

QString ProjectGroup::object() const { return d->object; }
void ProjectGroup::setObject(const QString &object) { d->object = object; }

QString ProjectGroup::projectId() const { return d->projectId; }
void ProjectGroup::setProjectId(const QString &projectId) { d->projectId = projectId; }

QString ProjectGroup::groupId() const { return d->groupId; }
void ProjectGroup::setGroupId(const QString &groupId) { d->groupId = groupId; }

QString ProjectGroup::groupName() const { return d->groupName; }
void ProjectGroup::setGroupName(const QString &groupName) { d->groupName = groupName; }

QString ProjectGroup::groupType() const { return d->groupType; }
void ProjectGroup::setGroupType(const QString &groupType) { d->groupType = groupType; }

qint64 ProjectGroup::createdAt() const { return d->createdAt; }
void ProjectGroup::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

bool ProjectGroup::isDeleted() const { return d->deleted; }
void ProjectGroup::setDeleted(bool deleted) { d->deleted = deleted; }

QJsonObject ProjectGroup::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("project_id"), d->projectId);
    detail::insertIfNotEmpty(json, QStringLiteral("group_id"), d->groupId);
    detail::insertIfNotEmpty(json, QStringLiteral("group_name"), d->groupName);
    detail::insertIfNotEmpty(json, QStringLiteral("group_type"), d->groupType);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfTrue(json, QStringLiteral("deleted"), d->deleted);
    return json;
}

ProjectGroup ProjectGroup::fromJson(const QJsonObject &json)
{
    ProjectGroup group;
    group.d->object = detail::stringOr(json, QStringLiteral("object"));
    group.d->projectId = detail::stringOr(json, QStringLiteral("project_id"));
    group.d->groupId = detail::stringOr(json, QStringLiteral("group_id"));
    group.d->groupName = detail::stringOr(json, QStringLiteral("group_name"));
    group.d->groupType = detail::stringOr(json, QStringLiteral("group_type"));
    group.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    group.d->deleted = json.value(QStringLiteral("deleted")).toBool();
    return group;
}

bool ProjectGroup::operator==(const ProjectGroup &other) const
{
    return d->object == other.d->object && d->projectId == other.d->projectId
           && d->groupId == other.d->groupId && d->groupName == other.d->groupName
           && d->groupType == other.d->groupType && d->createdAt == other.d->createdAt
           && d->deleted == other.d->deleted;
}

} // namespace Core
} // namespace QtOpenAi
