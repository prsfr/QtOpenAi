// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/RoleAssignment.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class RoleAssignmentData : public QSharedData
{
public:
    QString object;
    Group group;
    OrganizationUser user;
    OrganizationRole role;
    bool deleted = false;
};

RoleAssignment::RoleAssignment()
    : d(new RoleAssignmentData)
{ }

RoleAssignment::RoleAssignment(const RoleAssignment &other) = default;
RoleAssignment::RoleAssignment(RoleAssignment &&other) noexcept = default;
RoleAssignment &RoleAssignment::operator=(const RoleAssignment &other) = default;
RoleAssignment &RoleAssignment::operator=(RoleAssignment &&other) noexcept = default;
RoleAssignment::~RoleAssignment() = default;

QString RoleAssignment::object() const { return d->object; }
void RoleAssignment::setObject(const QString &object) { d->object = object; }

Group RoleAssignment::group() const { return d->group; }
void RoleAssignment::setGroup(const Group &group) { d->group = group; }

OrganizationUser RoleAssignment::user() const { return d->user; }
void RoleAssignment::setUser(const OrganizationUser &user) { d->user = user; }

OrganizationRole RoleAssignment::role() const { return d->role; }
void RoleAssignment::setRole(const OrganizationRole &role) { d->role = role; }

QString RoleAssignment::principalName() const
{
    // Whichever side is filled in, without asking the caller to branch. Falls
    // back to the other rather than to the empty string, as ApiKeyOwner::name()
    // does, so an `object` this build does not recognise still shows the name
    // the server sent.
    return d->group.name().isEmpty() ? d->user.name() : d->group.name();
}

bool RoleAssignment::isDeleted() const { return d->deleted; }
void RoleAssignment::setDeleted(bool deleted) { d->deleted = deleted; }

QJsonObject RoleAssignment::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    // Only the side that has something in it: the API sends one of the two and
    // an absent key says the same thing as a null one.
    const QJsonObject group = d->group.toJson();
    if (!group.isEmpty())
        json.insert(QStringLiteral("group"), group);
    const QJsonObject user = d->user.toJson();
    if (!user.isEmpty())
        json.insert(QStringLiteral("user"), user);
    const QJsonObject role = d->role.toJson();
    if (!role.isEmpty())
        json.insert(QStringLiteral("role"), role);
    detail::insertIfTrue(json, QStringLiteral("deleted"), d->deleted);
    return json;
}

RoleAssignment RoleAssignment::fromJson(const QJsonObject &json)
{
    RoleAssignment assignment;
    assignment.d->object = detail::stringOr(json, QStringLiteral("object"));
    assignment.d->group = Group::fromJson(json.value(QStringLiteral("group")).toObject());
    assignment.d->user = OrganizationUser::fromJson(json.value(QStringLiteral("user")).toObject());
    assignment.d->role = OrganizationRole::fromJson(json.value(QStringLiteral("role")).toObject());
    assignment.d->deleted = json.value(QStringLiteral("deleted")).toBool();
    return assignment;
}

bool RoleAssignment::operator==(const RoleAssignment &other) const
{
    return d->object == other.d->object && d->group == other.d->group && d->user == other.d->user
           && d->role == other.d->role && d->deleted == other.d->deleted;
}

} // namespace Core
} // namespace QtOpenAi
