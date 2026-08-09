// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>

#include <QtCore/QString>

#include <utility>

namespace QtOpenAi {
namespace Admin {

// Whether a role endpoint means the organization's roles or one project's.
//
//     organization.listRoles();                              // the organization
//     organization.listRoles(RoleScope::project("proj_1"));  // that project
//
// **The two scopes are one set of endpoints, not two.** Every role path exists
// twice — once for the organization and once for a project — and the OpenAPI
// document does not merely repeat the schemas across the pair, it points both at
// the same ones: the same Core::OrganizationRole, the same request body, the
// same assignment reply. Writing them out twice would have doubled thirteen
// methods and every test covering them, so that the second copy could differ
// from the first by a comment. Scope is a value the caller passes, and the path
// is the only thing it changes.
//
// **The prefix is not the one you would guess.** A project's *groups* live under
// `/organization/projects/{id}/groups`, but a project's *roles* live under
// `/projects/{id}/roles` — no `/organization` in front. That is what the API
// serves, in the path table and in its own curl examples alike, and it is the
// reason this is a type rather than a bool: a caller building the path by hand
// would put `/organization` there, and a wrong path is a 404 rather than a wrong
// answer.
//
// Default-constructed means the organization, so the common call takes no
// argument at all.
class QTOPENAI_ADMIN_EXPORT RoleScope
{
public:
    // The organization's own roles.
    RoleScope() = default;

    // One project's roles. Explicit so a bare project id cannot become a scope
    // by accident at a call site that meant something else.
    explicit RoleScope(QString projectId)
        : m_projectId(std::move(projectId))
    { }

    // Named constructors, for call sites where `RoleScope()` would read as
    // "no scope" rather than as "the organization".
    static RoleScope organization() { return RoleScope(); }
    static RoleScope project(const QString &projectId) { return RoleScope(projectId); }

    bool isOrganization() const { return m_projectId.isEmpty(); }
    bool isProject() const { return !m_projectId.isEmpty(); }

    // Empty at organization scope.
    QString projectId() const { return m_projectId; }

    bool operator==(const RoleScope &other) const { return m_projectId == other.m_projectId; }
    bool operator!=(const RoleScope &other) const { return !(*this == other); }

private:
    QString m_projectId;
};

} // namespace Admin
} // namespace QtOpenAi
