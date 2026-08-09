// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/CursorPage.h>
#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class GroupData;

// A group of people in the organization (GET/POST /organization/groups,
// GET/POST/DELETE /organization/groups/{group_id}).
//
// Part of the administration surface, which uses an admin API key rather than a
// standard one — see QtOpenAi::Admin::Organization.
//
// A group is the unit roles are granted to. Granting one to a group rather than
// to each member is the difference between revoking access once and revoking it
// n times, and it is why an inherited role is worth reporting — see
// Core::AssignmentSource.
//
// **A SCIM-managed group is read-only here.** `isScimManaged()` means the
// membership comes from an identity provider: adding or removing a member
// through this API will be undone by the next sync, which is a confusing way to
// find out. Check the flag before offering the button.
//
// The deletion acknowledgement decodes into this type as well, keeping the id
// and reporting the object as "group.deleted"; see isDeleted().
class QTOPENAI_CORE_EXPORT Group
{
public:
    Group();
    Group(const Group &other);
    Group(Group &&other) noexcept;
    Group &operator=(const Group &other);
    Group &operator=(Group &&other) noexcept;
    ~Group();

    void swap(Group &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "group" (or "group.deleted"). Several of the
    // group responses send no `object` at all, so an empty one is not an error.
    QString object() const;
    void setObject(const QString &object);

    QString name() const;
    void setName(const QString &name);

    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    // Whether the group is synchronised from an identity provider.
    //
    // **The API spells this field two ways.** A group read from the groups
    // endpoints carries `is_scim_managed`; the same group embedded in a role
    // assignment carries `scim_managed`. Both are read here and the longer one
    // is written, so a group does not silently arrive unmanaged just because it
    // came back attached to an assignment — which is exactly the case where
    // getting it wrong would offer an edit that the next sync reverts.
    bool isScimManaged() const;
    void setScimManaged(bool scimManaged);

    // "group" or "tenant_group". A free string, like every other discriminator
    // on this surface.
    QString groupType() const;
    void setGroupType(const QString &groupType);

    // True in the answer to DELETE /organization/groups/{group_id}, and false
    // everywhere else.
    bool isDeleted() const;
    void setDeleted(bool deleted);

    QJsonObject toJson() const;
    static Group fromJson(const QJsonObject &json);

    bool operator==(const Group &other) const;
    bool operator!=(const Group &other) const { return !(*this == other); }

private:
    QSharedDataPointer<GroupData> d;
};

// A page of groups (GET /organization/groups). Paginated by an opaque `next`
// cursor rather than by item ids — see CursorPage.
using GroupList = CursorPage<Group>;

class GroupMemberData;

// A person as seen through a group's membership
// (GET /organization/groups/{group_id}/users, GET .../{user_id}).
//
// **Not Core::OrganizationUser, unlike a project's members.** A project member
// was the same six fields as an organization member, so #101 used one class for
// both. This is not that case: a group member has a picture, a service-account
// flag and a user type, and has neither the organization `role` nor the
// `added_at` that make an OrganizationUser worth having. Three fields overlap
// and three differ each way, so one class would have meant half its accessors
// returning nothing whichever endpoint filled it in.
//
// The listing sends only `id`, `name` and `email`; the single-member read adds
// the rest. That is the same type either way — a field the server left out is
// absent, not different.
class QTOPENAI_CORE_EXPORT GroupMember
{
public:
    GroupMember();
    GroupMember(const GroupMember &other);
    GroupMember(GroupMember &&other) noexcept;
    GroupMember &operator=(const GroupMember &other);
    GroupMember &operator=(GroupMember &&other) noexcept;
    ~GroupMember();

    void swap(GroupMember &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    QString name() const;
    void setName(const QString &name);

    // Empty for a principal that has none — a service account, typically.
    QString email() const;
    void setEmail(const QString &email);

    // URL of the profile picture, when the server sent one.
    QString picture() const;
    void setPicture(const QString &picture);

    bool isServiceAccount() const;
    void setServiceAccount(bool serviceAccount);

    // "user" or "tenant_user"; empty in the listing, which does not send it.
    QString userType() const;
    void setUserType(const QString &userType);

    QJsonObject toJson() const;
    static GroupMember fromJson(const QJsonObject &json);

    bool operator==(const GroupMember &other) const;
    bool operator!=(const GroupMember &other) const { return !(*this == other); }

private:
    QSharedDataPointer<GroupMemberData> d;
};

using GroupMemberList = CursorPage<GroupMember>;

class GroupMembershipData;

// The acknowledgement of a change to who is in a group (POST/DELETE
// /organization/groups/{group_id}/users[/{user_id}]).
//
// **It is not the member.** Adding someone answers with the two ids and nothing
// else — no name, no email — and removing someone answers with `deleted` alone.
// Decoding either into a GroupMember would have produced a person with an id and
// no name, which reads like a member whose fields went missing rather than like
// an acknowledgement. The ids the caller already knows are echoed back because
// that is what the API sends, and they make the reply worth logging.
class QTOPENAI_CORE_EXPORT GroupMembership
{
public:
    GroupMembership();
    GroupMembership(const GroupMembership &other);
    GroupMembership(GroupMembership &&other) noexcept;
    GroupMembership &operator=(const GroupMembership &other);
    GroupMembership &operator=(GroupMembership &&other) noexcept;
    ~GroupMembership();

    void swap(GroupMembership &other) noexcept { d.swap(other.d); }

    // "group.user" or "group.user.deleted".
    QString object() const;
    void setObject(const QString &object);

    // Both empty in the removal acknowledgement, which sends neither.
    QString groupId() const;
    void setGroupId(const QString &groupId);

    QString userId() const;
    void setUserId(const QString &userId);

    bool isDeleted() const;
    void setDeleted(bool deleted);

    QJsonObject toJson() const;
    static GroupMembership fromJson(const QJsonObject &json);

    bool operator==(const GroupMembership &other) const;
    bool operator!=(const GroupMembership &other) const { return !(*this == other); }

private:
    QSharedDataPointer<GroupMembershipData> d;
};

class ProjectGroupData;

// A group's access to a project (GET/POST
// /organization/projects/{project_id}/groups, GET/DELETE .../{group_id}).
//
// **A flattened membership, not a Group.** The server sends the group's id and
// name inline as `group_id` and `group_name` rather than a nested group object,
// and adds the project id and the moment access was granted. Decoding it into a
// Group would have meant a Group whose `id` is sometimes the membership and
// sometimes the group; keeping the API's shape keeps the two apart.
//
// The removal acknowledgement decodes into this type as well, reporting the
// object as "project.group.deleted" and carrying no ids at all.
class QTOPENAI_CORE_EXPORT ProjectGroup
{
public:
    ProjectGroup();
    ProjectGroup(const ProjectGroup &other);
    ProjectGroup(ProjectGroup &&other) noexcept;
    ProjectGroup &operator=(const ProjectGroup &other);
    ProjectGroup &operator=(ProjectGroup &&other) noexcept;
    ~ProjectGroup();

    void swap(ProjectGroup &other) noexcept { d.swap(other.d); }

    // "project.group" or "project.group.deleted".
    QString object() const;
    void setObject(const QString &object);

    QString projectId() const;
    void setProjectId(const QString &projectId);

    QString groupId() const;
    void setGroupId(const QString &groupId);

    QString groupName() const;
    void setGroupName(const QString &groupName);

    QString groupType() const;
    void setGroupType(const QString &groupType);

    // Unix timestamp of when the group was granted access to the project — not
    // of when the group itself was created.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    bool isDeleted() const;
    void setDeleted(bool deleted);

    QJsonObject toJson() const;
    static ProjectGroup fromJson(const QJsonObject &json);

    bool operator==(const ProjectGroup &other) const;
    bool operator!=(const ProjectGroup &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ProjectGroupData> d;
};

using ProjectGroupList = CursorPage<ProjectGroup>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Group)
Q_DECLARE_SHARED(QtOpenAi::Core::GroupMember)
Q_DECLARE_SHARED(QtOpenAi::Core::GroupMembership)
Q_DECLARE_SHARED(QtOpenAi::Core::ProjectGroup)
Q_DECLARE_METATYPE(QtOpenAi::Core::Group)
Q_DECLARE_METATYPE(QtOpenAi::Core::GroupList)
Q_DECLARE_METATYPE(QtOpenAi::Core::GroupMember)
Q_DECLARE_METATYPE(QtOpenAi::Core::GroupMemberList)
Q_DECLARE_METATYPE(QtOpenAi::Core::GroupMembership)
Q_DECLARE_METATYPE(QtOpenAi::Core::ProjectGroup)
Q_DECLARE_METATYPE(QtOpenAi::Core::ProjectGroupList)
