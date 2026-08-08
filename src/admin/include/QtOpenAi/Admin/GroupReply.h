// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>
#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Group.h>

namespace QtOpenAi {

namespace Client {
// Declared here so the friendships below can name it; including Client.h would
// pull ~70 reply headers into every user of this one.
class Client;
} // namespace Client

namespace Admin {

// An asynchronous handle for one group (GET/POST/DELETE
// /organization/groups/{group_id}). See Client::RestReplyBase for the shared
// lifecycle.
//
// The reply types of this module derive from the same TypedReply<T> as every
// other endpoint's, and are created through the same request path -- see
// Organization. The friend declaration names Client::Client because that is
// where replies are constructed, so that a reply still cannot be created from
// outside the library.
//
// The deletion acknowledgement decodes into Core::Group as well, keeping the id
// and reporting the object as "group.deleted".
class QTOPENAI_ADMIN_EXPORT GroupReply : public Client::TypedReply<Core::Group>
{
    Q_OBJECT
public:
    Core::Group group() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Group &group);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::Group &group) override { Q_EMIT finished(group); }
};

// An asynchronous handle for one page of groups (GET /organization/groups).
class QTOPENAI_ADMIN_EXPORT GroupListReply : public Client::TypedReply<Core::GroupList>
{
    Q_OBJECT
public:
    Core::GroupList groups() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::GroupList &groups);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::GroupList &groups) override { Q_EMIT finished(groups); }
};

// An asynchronous handle for one member of a group
// (GET /organization/groups/{group_id}/users/{user_id}).
class QTOPENAI_ADMIN_EXPORT GroupMemberReply : public Client::TypedReply<Core::GroupMember>
{
    Q_OBJECT
public:
    Core::GroupMember member() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::GroupMember &member);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::GroupMember &member) override { Q_EMIT finished(member); }
};

// An asynchronous handle for one page of a group's members
// (GET /organization/groups/{group_id}/users).
class QTOPENAI_ADMIN_EXPORT GroupMemberListReply : public Client::TypedReply<Core::GroupMemberList>
{
    Q_OBJECT
public:
    Core::GroupMemberList members() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::GroupMemberList &members);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::GroupMemberList &members) override { Q_EMIT finished(members); }
};

// An asynchronous handle for adding someone to a group or removing them
// (POST/DELETE /organization/groups/{group_id}/users[/{user_id}]). Not the
// member -- see Core::GroupMembership for what the API answers with.
class QTOPENAI_ADMIN_EXPORT GroupMembershipReply : public Client::TypedReply<Core::GroupMembership>
{
    Q_OBJECT
public:
    Core::GroupMembership membership() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::GroupMembership &membership);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::GroupMembership &membership) override
    {
        Q_EMIT finished(membership);
    }
};

// An asynchronous handle for one group's access to a project (GET/POST/DELETE
// /organization/projects/{project_id}/groups[/{group_id}]).
class QTOPENAI_ADMIN_EXPORT ProjectGroupReply : public Client::TypedReply<Core::ProjectGroup>
{
    Q_OBJECT
public:
    Core::ProjectGroup projectGroup() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ProjectGroup &projectGroup);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ProjectGroup &projectGroup) override
    {
        Q_EMIT finished(projectGroup);
    }
};

// An asynchronous handle for one page of the groups with access to a project
// (GET /organization/projects/{project_id}/groups).
class QTOPENAI_ADMIN_EXPORT ProjectGroupListReply
    : public Client::TypedReply<Core::ProjectGroupList>
{
    Q_OBJECT
public:
    Core::ProjectGroupList projectGroups() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ProjectGroupList &projectGroups);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ProjectGroupList &projectGroups) override
    {
        Q_EMIT finished(projectGroups);
    }
};

} // namespace Admin
} // namespace QtOpenAi
