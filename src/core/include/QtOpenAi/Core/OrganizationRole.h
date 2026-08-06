// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/CursorPage.h>
#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/OrganizationUser.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace QtOpenAi {
namespace Core {

// Why a principal holds a role it was never directly given.
//
// A user can end up with a role because a group they belong to has it. The
// assignment listing reports that in `assignment_sources`, and it is the field
// that answers the only interesting question about an inherited role: *who do I
// take this away from?* Removing the assignment from the user does nothing when
// the user never had one.
//
// A small aggregate rather than an implicitly shared class, like
// ServiceAccountApiKey: it is only ever read as part of an OrganizationRole.
struct QTOPENAI_CORE_EXPORT AssignmentSource
{
    QString principalId;
    QString principalType; // "group" or "user"

    bool isGroup() const { return principalType == QLatin1String("group"); }

    QJsonObject toJson() const;
    static AssignmentSource fromJson(const QJsonObject &json);

    bool operator==(const AssignmentSource &other) const
    {
        return principalId == other.principalId && principalType == other.principalType;
    }
    bool operator!=(const AssignmentSource &other) const { return !(*this == other); }
};

class OrganizationRoleData;

// A role: a named set of permissions that can be granted to a user or a group
// (GET/POST /organization/roles, GET/POST/DELETE /organization/roles/{role_id},
// and the project-scoped mirror of all five under /projects/{project_id}/roles).
//
// Part of the administration surface, which uses an admin API key rather than a
// standard one — see QtOpenAi::Admin::Organization.
//
// Named for the surface rather than shortened to `Role`, exactly as
// OrganizationUser is: `Core::Role` is already taken, by the enum naming the
// author of a chat message. The prefix says which of the two a reader is looking
// at, and it says nothing about scope — a project's roles are this class too,
// with a different `resourceType()`.
//
// **One class for both scopes, because the payload really is the same one.** An
// organization role and a project role differ in `resource_type`
// ("api.organization" against "api.project") and in nothing else; the OpenAPI
// document does not merely repeat the schema for the two, it points both at this
// single type. A second class would have been a copy that differs by a comment,
// and every caller that lists roles for a screen would then need two of them.
// Which scope a role belongs to is a value, not a type — see Admin::RoleScope.
//
// **One class for the catalogue and for an assignment, too.** Listing the roles
// a principal holds returns the same fields plus provenance: when the role was
// made, by whom, and — `assignmentSources()` — which group it is inherited
// through. Reading a role out of the catalogue simply leaves those empty. They
// are marked below.
//
// The deletion acknowledgement decodes into this type as well, keeping the id
// and reporting the object as "role.deleted"; see isDeleted().
class QTOPENAI_CORE_EXPORT OrganizationRole
{
public:
    OrganizationRole();
    OrganizationRole(const OrganizationRole &other);
    OrganizationRole(OrganizationRole &&other) noexcept;
    OrganizationRole &operator=(const OrganizationRole &other);
    OrganizationRole &operator=(OrganizationRole &&other) noexcept;
    ~OrganizationRole();

    void swap(OrganizationRole &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "role" (or "role.deleted"). Absent from the
    // assignment listings, which send the fields without an envelope.
    QString object() const;
    void setObject(const QString &object);

    QString name() const;
    void setName(const QString &name);

    QString description() const;
    void setDescription(const QString &description);

    // The permission strings the role grants, e.g. "api.groups.read". Free
    // strings rather than an enum, as every other vocabulary on this surface is:
    // a permission this build has never heard of has to survive a round trip
    // rather than decay to the first enumerator — and getting that wrong here
    // would quietly widen or narrow what a role can do.
    QStringList permissions() const;
    void setPermissions(const QStringList &permissions);

    // What the role is bound to: "api.organization" or "api.project". This is
    // the one field that distinguishes an organization role from a project role.
    QString resourceType() const;
    void setResourceType(const QString &resourceType);

    bool isProjectScoped() const { return resourceType() == QLatin1String("api.project"); }

    // True for the roles OpenAI ships and maintains. They cannot be updated or
    // deleted, which is worth knowing before offering the button.
    bool predefinedRole() const;
    void setPredefinedRole(bool predefinedRole);

    // --- Present only when the role was read as an assignment ---------------
    // Unix timestamps; 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    qint64 updatedAt() const;
    void setUpdatedAt(qint64 updatedAt);

    // The id of whoever created the role, and the same person expanded when the
    // server chose to send it. `created_by_user_obj` is null more often than not,
    // so createdBy() is the field to rely on.
    QString createdBy() const;
    void setCreatedBy(const QString &createdBy);

    OrganizationUser createdByUser() const;
    void setCreatedByUser(const OrganizationUser &createdByUser);

    // Arbitrary server-side metadata, kept as the object it arrived as: this
    // library has no schema for it and inventing one would drop keys.
    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    // Empty for a role held directly. See AssignmentSource.
    QList<AssignmentSource> assignmentSources() const;
    void setAssignmentSources(const QList<AssignmentSource> &sources);

    bool isInherited() const { return !assignmentSources().isEmpty(); }

    // --- Deletion acknowledgement ------------------------------------------
    // True in the answer to DELETE .../roles/{role_id}, and false everywhere
    // else. Read from the payload's `deleted` rather than inferred from the
    // object name, so a server that reports a refused deletion as
    // `{"object":"role.deleted","deleted":false}` is believed.
    bool isDeleted() const;
    void setDeleted(bool deleted);

    QJsonObject toJson() const;
    static OrganizationRole fromJson(const QJsonObject &json);

    bool operator==(const OrganizationRole &other) const;
    bool operator!=(const OrganizationRole &other) const { return !(*this == other); }

private:
    QSharedDataPointer<OrganizationRoleData> d;
};

// A page of roles, whether the catalogue of them (GET .../roles) or the ones a
// principal holds (GET .../{principal}/roles). Paginated by an opaque `next`
// cursor rather than by item ids — see CursorPage.
using OrganizationRoleList = CursorPage<OrganizationRole>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::OrganizationRole)
Q_DECLARE_METATYPE(QtOpenAi::Core::AssignmentSource)
Q_DECLARE_METATYPE(QtOpenAi::Core::OrganizationRole)
Q_DECLARE_METATYPE(QtOpenAi::Core::OrganizationRoleList)
