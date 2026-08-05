// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

// One project an invitation grants access to, and the role it grants there.
//
// A small aggregate rather than an implicitly shared class, like CostAmount: two
// members, no allocation to save, and it is only ever read as part of an Invite.
//
// The project role ("member" or "owner") is *not* the organization role on
// Invite and OrganizationUser. A reader in the organization can still own a
// project, which is exactly the distinction that makes flattening these two into
// one field a bad idea.
struct QTOPENAI_CORE_EXPORT InviteProject
{
    QString id;
    QString role;

    QJsonObject toJson() const;
    static InviteProject fromJson(const QJsonObject &json);

    bool operator==(const InviteProject &other) const
    {
        return id == other.id && role == other.role;
    }
    bool operator!=(const InviteProject &other) const { return !(*this == other); }
};

class InviteData;

// An invitation to join the organization (GET/POST /organization/invites,
// GET/DELETE /organization/invites/{invite_id}).
//
// Part of the administration surface, which uses an admin API key rather than a
// standard one — see QtOpenAi::Admin::Organization.
//
// An invite is the *pending* half of membership: it exists until it is accepted,
// expires, or is deleted, and only then does an OrganizationUser appear. The
// three timestamps below are what tells those states apart, which is why they
// are kept separate from `status` rather than derived from it.
//
// The deletion acknowledgement of DELETE /organization/invites/{invite_id} also
// decodes into this type; it keeps the id in `id` and reports the object as
// "organization.invite.deleted".
class QTOPENAI_CORE_EXPORT Invite
{
public:
    Invite();
    Invite(const Invite &other);
    Invite(Invite &&other) noexcept;
    Invite &operator=(const Invite &other);
    Invite &operator=(Invite &&other) noexcept;
    ~Invite();

    void swap(Invite &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "organization.invite" (or
    // "organization.invite.deleted").
    QString object() const;
    void setObject(const QString &object);

    QString email() const;
    void setEmail(const QString &email);

    // The organization role the invitation grants, "owner" or "reader". A string
    // for the same reason OrganizationUser::role is.
    QString role() const;
    void setRole(const QString &role);

    // "pending", "accepted" or "expired". Also a string, and also deliberately:
    // a status this build has never heard of has to survive a round trip.
    QString status() const;
    void setStatus(const QString &status);

    bool isAccepted() const { return status() == QLatin1String("accepted"); }

    // Unix timestamps; 0 when absent. `acceptedAt` stays 0 until someone
    // accepts, which is the normal state of a live invitation rather than an
    // error.
    qint64 invitedAt() const;
    void setInvitedAt(qint64 invitedAt);

    qint64 expiresAt() const;
    void setExpiresAt(qint64 expiresAt);

    qint64 acceptedAt() const;
    void setAcceptedAt(qint64 acceptedAt);

    // The projects the invitation grants access to, each with its own role.
    // Empty means organization membership without any project attached.
    QList<InviteProject> projects() const;
    void setProjects(const QList<InviteProject> &projects);

    QJsonObject toJson() const;
    static Invite fromJson(const QJsonObject &json);

    bool operator==(const Invite &other) const;
    bool operator!=(const Invite &other) const { return !(*this == other); }

private:
    QSharedDataPointer<InviteData> d;
};

// A `list` of invitations (GET /organization/invites). Cursor-paginated; reuses
// the shared list-page type.
using InviteList = ListPage<Invite>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Invite)
Q_DECLARE_METATYPE(QtOpenAi::Core::InviteProject)
Q_DECLARE_METATYPE(QtOpenAi::Core::Invite)
Q_DECLARE_METATYPE(QtOpenAi::Core::InviteList)
