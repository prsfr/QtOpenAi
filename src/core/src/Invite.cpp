// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Invite.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

QJsonObject InviteProject::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), id);
    detail::insertIfNotEmpty(json, QStringLiteral("role"), role);
    return json;
}

InviteProject InviteProject::fromJson(const QJsonObject &json)
{
    InviteProject project;
    project.id = detail::stringOr(json, QStringLiteral("id"));
    project.role = detail::stringOr(json, QStringLiteral("role"));
    return project;
}

class InviteData : public QSharedData
{
public:
    QString id;
    QString object;
    QString email;
    QString role;
    QString status;
    qint64 invitedAt = 0;
    qint64 expiresAt = 0;
    qint64 acceptedAt = 0;
    QList<InviteProject> projects;
};

Invite::Invite()
    : d(new InviteData)
{ }

Invite::Invite(const Invite &other) = default;
Invite::Invite(Invite &&other) noexcept = default;
Invite &Invite::operator=(const Invite &other) = default;
Invite &Invite::operator=(Invite &&other) noexcept = default;
Invite::~Invite() = default;

QString Invite::id() const { return d->id; }
void Invite::setId(const QString &id) { d->id = id; }

QString Invite::object() const { return d->object; }
void Invite::setObject(const QString &object) { d->object = object; }

QString Invite::email() const { return d->email; }
void Invite::setEmail(const QString &email) { d->email = email; }

QString Invite::role() const { return d->role; }
void Invite::setRole(const QString &role) { d->role = role; }

QString Invite::status() const { return d->status; }
void Invite::setStatus(const QString &status) { d->status = status; }

qint64 Invite::invitedAt() const { return d->invitedAt; }
void Invite::setInvitedAt(qint64 invitedAt) { d->invitedAt = invitedAt; }

qint64 Invite::expiresAt() const { return d->expiresAt; }
void Invite::setExpiresAt(qint64 expiresAt) { d->expiresAt = expiresAt; }

qint64 Invite::acceptedAt() const { return d->acceptedAt; }
void Invite::setAcceptedAt(qint64 acceptedAt) { d->acceptedAt = acceptedAt; }

QList<InviteProject> Invite::projects() const { return d->projects; }
void Invite::setProjects(const QList<InviteProject> &projects) { d->projects = projects; }

QJsonObject Invite::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("email"), d->email);
    detail::insertIfNotEmpty(json, QStringLiteral("role"), d->role);
    detail::insertIfNotEmpty(json, QStringLiteral("status"), d->status);
    detail::insertIfNonZero(json, QStringLiteral("invited_at"), d->invitedAt);
    detail::insertIfNonZero(json, QStringLiteral("expires_at"), d->expiresAt);
    // Left out rather than written as a zero: an unaccepted invitation has no
    // acceptance time, and 1970 is not a better answer than silence.
    detail::insertIfNonZero(json, QStringLiteral("accepted_at"), d->acceptedAt);
    if (!d->projects.isEmpty()) {
        QJsonArray projects;
        for (const InviteProject &project : d->projects)
            projects.append(project.toJson());
        json.insert(QStringLiteral("projects"), projects);
    }
    return json;
}

Invite Invite::fromJson(const QJsonObject &json)
{
    Invite invite;
    invite.d->id = detail::stringOr(json, QStringLiteral("id"));
    invite.d->object = detail::stringOr(json, QStringLiteral("object"));
    invite.d->email = detail::stringOr(json, QStringLiteral("email"));
    invite.d->role = detail::stringOr(json, QStringLiteral("role"));
    invite.d->status = detail::stringOr(json, QStringLiteral("status"));
    invite.d->invitedAt = detail::int64Or(json, QStringLiteral("invited_at"));
    invite.d->expiresAt = detail::int64Or(json, QStringLiteral("expires_at"));
    invite.d->acceptedAt = detail::int64Or(json, QStringLiteral("accepted_at"));
    const QJsonArray projects = json.value(QStringLiteral("projects")).toArray();
    invite.d->projects.reserve(projects.size());
    for (const QJsonValue &project : projects)
        invite.d->projects.append(InviteProject::fromJson(project.toObject()));
    return invite;
}

bool Invite::operator==(const Invite &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->email == other.d->email
           && d->role == other.d->role && d->status == other.d->status
           && d->invitedAt == other.d->invitedAt && d->expiresAt == other.d->expiresAt
           && d->acceptedAt == other.d->acceptedAt && d->projects == other.d->projects;
}

} // namespace Core
} // namespace QtOpenAi
