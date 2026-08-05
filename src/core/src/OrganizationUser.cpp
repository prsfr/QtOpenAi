// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/OrganizationUser.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class OrganizationUserData : public QSharedData
{
public:
    QString id;
    QString object;
    QString name;
    QString email;
    QString role;
    qint64 addedAt = 0;
};

OrganizationUser::OrganizationUser()
    : d(new OrganizationUserData)
{ }

OrganizationUser::OrganizationUser(const OrganizationUser &other) = default;
OrganizationUser::OrganizationUser(OrganizationUser &&other) noexcept = default;
OrganizationUser &OrganizationUser::operator=(const OrganizationUser &other) = default;
OrganizationUser &OrganizationUser::operator=(OrganizationUser &&other) noexcept = default;
OrganizationUser::~OrganizationUser() = default;

QString OrganizationUser::id() const { return d->id; }
void OrganizationUser::setId(const QString &id) { d->id = id; }

QString OrganizationUser::object() const { return d->object; }
void OrganizationUser::setObject(const QString &object) { d->object = object; }

QString OrganizationUser::name() const { return d->name; }
void OrganizationUser::setName(const QString &name) { d->name = name; }

QString OrganizationUser::email() const { return d->email; }
void OrganizationUser::setEmail(const QString &email) { d->email = email; }

QString OrganizationUser::role() const { return d->role; }
void OrganizationUser::setRole(const QString &role) { d->role = role; }

qint64 OrganizationUser::addedAt() const { return d->addedAt; }
void OrganizationUser::setAddedAt(qint64 addedAt) { d->addedAt = addedAt; }

QJsonObject OrganizationUser::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNotEmpty(json, QStringLiteral("email"), d->email);
    detail::insertIfNotEmpty(json, QStringLiteral("role"), d->role);
    detail::insertIfNonZero(json, QStringLiteral("added_at"), d->addedAt);
    return json;
}

OrganizationUser OrganizationUser::fromJson(const QJsonObject &json)
{
    OrganizationUser user;
    user.d->id = detail::stringOr(json, QStringLiteral("id"));
    user.d->object = detail::stringOr(json, QStringLiteral("object"));
    user.d->name = detail::stringOr(json, QStringLiteral("name"));
    user.d->email = detail::stringOr(json, QStringLiteral("email"));
    user.d->role = detail::stringOr(json, QStringLiteral("role"));
    user.d->addedAt = detail::int64Or(json, QStringLiteral("added_at"));
    return user;
}

bool OrganizationUser::operator==(const OrganizationUser &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->name == other.d->name
           && d->email == other.d->email && d->role == other.d->role
           && d->addedAt == other.d->addedAt;
}

} // namespace Core
} // namespace QtOpenAi
