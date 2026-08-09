// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/Group.h>
#include <QtOpenAi/Core/OrganizationRole.h>
#include <QtOpenAi/Core/OrganizationUser.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class RoleAssignmentData;

// The grant of a role to a principal, and the acknowledgement of taking it away
// (POST/DELETE .../groups/{group_id}/roles[/{role_id}] and
// .../users/{user_id}/roles[/{role_id}], at organization and project scope
// alike).
//
// **One class for four endpoints, because the API sends one shape.** Assigning a
// role to a group answers with `{object: "group.role", group, role}` and
// assigning one to a user answers with `{object: "user.role", user, role}` —
// the same envelope with a different principal, exactly the tagged union
// Core::ApiKeyOwner keeps for the owner of an API key. `object` says which side
// is filled in; isGroup() and isUser() ask it.
//
// Scope does not enter into it. A project-scoped assignment sends the same
// payload as an organization-scoped one; only the path differs, which is what
// Admin::RoleScope carries.
//
// **Unassigning answers with almost nothing**: `{object: "group.role.deleted",
// deleted: true}` — no ids, not even the role's. There is nothing else to
// decode, so the acknowledgement lands in this type with `role()` and the two
// principals default-constructed and isDeleted() true.
class QTOPENAI_CORE_EXPORT RoleAssignment
{
public:
    RoleAssignment();
    RoleAssignment(const RoleAssignment &other);
    RoleAssignment(RoleAssignment &&other) noexcept;
    RoleAssignment &operator=(const RoleAssignment &other);
    RoleAssignment &operator=(RoleAssignment &&other) noexcept;
    ~RoleAssignment();

    void swap(RoleAssignment &other) noexcept { d.swap(other.d); }

    // "group.role", "user.role", or either with ".deleted" appended.
    QString object() const;
    void setObject(const QString &object);

    bool isGroup() const { return object().startsWith(QLatin1String("group.role")); }
    bool isUser() const { return object().startsWith(QLatin1String("user.role")); }

    // Whichever of the two `object` names; the other is default-constructed.
    Group group() const;
    void setGroup(const Group &group);

    OrganizationUser user() const;
    void setUser(const OrganizationUser &user);

    // The role that was granted. Empty in an unassignment acknowledgement.
    OrganizationRole role() const;
    void setRole(const OrganizationRole &role);

    // The principal's display name whichever kind it is, for a log line or a
    // list column that does not care. Prefer the typed accessors when the
    // distinction matters.
    QString principalName() const;

    // True in the answer to DELETE, and false everywhere else.
    bool isDeleted() const;
    void setDeleted(bool deleted);

    QJsonObject toJson() const;
    static RoleAssignment fromJson(const QJsonObject &json);

    bool operator==(const RoleAssignment &other) const;
    bool operator!=(const RoleAssignment &other) const { return !(*this == other); }

private:
    QSharedDataPointer<RoleAssignmentData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::RoleAssignment)
Q_DECLARE_METATYPE(QtOpenAi::Core::RoleAssignment)
