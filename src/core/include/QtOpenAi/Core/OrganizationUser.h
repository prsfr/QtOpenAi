// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class OrganizationUserData;

// A member of the organization (GET /organization/users,
// GET/POST/DELETE /organization/users/{user_id}).
//
// Part of the administration surface, which uses an admin API key rather than a
// standard one — see QtOpenAi::Admin::Organization.
//
// Named for the OpenAPI type rather than shortened to `User` the way `Project`
// was: this library already has a "user" — the chat role, Message::user() — and
// a bare Core::User next to it would read as the person typing rather than as an
// account on the organization's bill.
//
// The deletion acknowledgement of DELETE /organization/users/{user_id} also
// decodes into this type; it keeps the id in `id` and reports the object as
// "organization.user.deleted".
class QTOPENAI_CORE_EXPORT OrganizationUser
{
public:
    OrganizationUser();
    OrganizationUser(const OrganizationUser &other);
    OrganizationUser(OrganizationUser &&other) noexcept;
    OrganizationUser &operator=(const OrganizationUser &other);
    OrganizationUser &operator=(OrganizationUser &&other) noexcept;
    ~OrganizationUser();

    void swap(OrganizationUser &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "organization.user" (or
    // "organization.user.deleted").
    QString object() const;
    void setObject(const QString &object);

    QString name() const;
    void setName(const QString &name);

    QString email() const;
    void setEmail(const QString &email);

    // The organization role, "owner" or "reader". Kept as the string the server
    // sent rather than an enum, the same as Project::status: a role this build
    // has never heard of has to survive a round trip rather than decay to the
    // first enumerator — and getting that wrong on *this* field would silently
    // report a reader as an owner.
    QString role() const;
    void setRole(const QString &role);

    bool isOwner() const { return role() == QLatin1String("owner"); }

    // Unix timestamp of when the user joined the organization (`added_at`);
    // 0 when absent.
    qint64 addedAt() const;
    void setAddedAt(qint64 addedAt);

    QJsonObject toJson() const;
    static OrganizationUser fromJson(const QJsonObject &json);

    bool operator==(const OrganizationUser &other) const;
    bool operator!=(const OrganizationUser &other) const { return !(*this == other); }

private:
    QSharedDataPointer<OrganizationUserData> d;
};

// A `list` of organization members (GET /organization/users). Cursor-paginated;
// reuses the shared list-page type.
using OrganizationUserList = ListPage<OrganizationUser>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::OrganizationUser)
Q_DECLARE_METATYPE(QtOpenAi::Core::OrganizationUser)
Q_DECLARE_METATYPE(QtOpenAi::Core::OrganizationUserList)
