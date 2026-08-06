// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>
#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/OrganizationRole.h>
#include <QtOpenAi/Core/RoleAssignment.h>

namespace QtOpenAi {

namespace Client {
// Declared here so the friendships below can name it; including Client.h would
// pull ~70 reply headers into every user of this one.
class Client;
} // namespace Client

namespace Admin {

// An asynchronous handle for one role (GET/POST/DELETE
// /organization/roles/{role_id}, /projects/{project_id}/roles/{role_id}, and the
// single-assignment reads under a group or a user). See Client::RestReplyBase
// for the shared lifecycle.
//
// The reply types of this module derive from the same TypedReply<T> as every
// other endpoint's, and are created through the same request path -- see
// Organization. The friend declaration names Client::Client because that is
// where replies are constructed, so that a reply still cannot be created from
// outside the library.
//
// One reply for both scopes, because the payload is the same one -- see
// RoleScope. The deletion acknowledgement decodes into Core::OrganizationRole
// as well.
class QTOPENAI_ADMIN_EXPORT RoleReply : public Client::TypedReply<Core::OrganizationRole>
{
    Q_OBJECT
public:
    Core::OrganizationRole role() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::OrganizationRole &role);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::OrganizationRole &role) override { Q_EMIT finished(role); }
};

// An asynchronous handle for one page of roles: the catalogue (GET .../roles) or
// the ones a principal holds (GET .../{principal_id}/roles), at either scope.
class QTOPENAI_ADMIN_EXPORT RoleListReply : public Client::TypedReply<Core::OrganizationRoleList>
{
    Q_OBJECT
public:
    Core::OrganizationRoleList roles() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::OrganizationRoleList &roles);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::OrganizationRoleList &roles) override { Q_EMIT finished(roles); }
};

// An asynchronous handle for granting a role to a group or a user, and for the
// acknowledgement of taking it back (POST/DELETE
// .../{principal_id}/roles[/{role_id}]). See Core::RoleAssignment for why one
// type covers both principals and both scopes.
class QTOPENAI_ADMIN_EXPORT RoleAssignmentReply : public Client::TypedReply<Core::RoleAssignment>
{
    Q_OBJECT
public:
    Core::RoleAssignment assignment() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::RoleAssignment &assignment);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::RoleAssignment &assignment) override
    {
        Q_EMIT finished(assignment);
    }
};

} // namespace Admin
} // namespace QtOpenAi
