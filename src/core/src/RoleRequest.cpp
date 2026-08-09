// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/RoleRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class RoleRequestData : public QSharedData
{
public:
    std::optional<QString> roleName;
    std::optional<QStringList> permissions;
    std::optional<QString> description;
};

RoleRequest::RoleRequest()
    : d(new RoleRequestData)
{ }

RoleRequest::RoleRequest(const RoleRequest &other) = default;
RoleRequest::RoleRequest(RoleRequest &&other) noexcept = default;
RoleRequest &RoleRequest::operator=(const RoleRequest &other) = default;
RoleRequest &RoleRequest::operator=(RoleRequest &&other) noexcept = default;
RoleRequest::~RoleRequest() = default;

std::optional<QString> RoleRequest::roleName() const { return d->roleName; }
void RoleRequest::setRoleName(const QString &roleName) { d->roleName = roleName; }

std::optional<QStringList> RoleRequest::permissions() const { return d->permissions; }
void RoleRequest::setPermissions(const QStringList &permissions) { d->permissions = permissions; }

std::optional<QString> RoleRequest::description() const { return d->description; }
void RoleRequest::setDescription(const QString &description) { d->description = description; }

bool RoleRequest::isEmpty() const { return !d->roleName && !d->permissions && !d->description; }

QJsonObject RoleRequest::toJson() const
{
    QJsonObject json;
    // `role_name`, not `name` -- see the declaration.
    detail::insertIfSet(json, QStringLiteral("role_name"), d->roleName);
    if (d->permissions) {
        // Written even when the list is empty, unlike the string fields of every
        // other request in this library: an explicitly empty `permissions` is
        // how a role's grants are revoked, and dropping it would silently turn
        // that into "leave them alone".
        json.insert(QStringLiteral("permissions"), QJsonArray::fromStringList(*d->permissions));
    }
    detail::insertIfSet(json, QStringLiteral("description"), d->description);
    return json;
}

RoleRequest RoleRequest::fromJson(const QJsonObject &json)
{
    RoleRequest request;
    if (json.contains(QStringLiteral("role_name")))
        request.d->roleName = detail::stringOr(json, QStringLiteral("role_name"));
    if (json.contains(QStringLiteral("permissions")))
        request.d->permissions = detail::stringListOr(json, QStringLiteral("permissions"));
    if (json.contains(QStringLiteral("description")))
        request.d->description = detail::stringOr(json, QStringLiteral("description"));
    return request;
}

bool RoleRequest::operator==(const RoleRequest &other) const
{
    return d->roleName == other.d->roleName && d->permissions == other.d->permissions
           && d->description == other.d->description;
}

} // namespace Core
} // namespace QtOpenAi
