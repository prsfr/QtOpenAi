// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <optional>

namespace QtOpenAi {
namespace Core {

class RoleRequestData;

// What to create a custom role with, or what to change about one (POST
// /organization/roles, POST /organization/roles/{role_id}, and the same two
// under /projects/{project_id}/roles).
//
//     Core::RoleRequest request;
//     request.setRoleName(QStringLiteral("API Group Manager"));
//     request.setPermissions({QStringLiteral("api.groups.read")});
//     organization.createRole(request);
//
// **One type for both directions, as Core::ProjectRateLimit is.** The API points
// creation and update at two schemas that differ only in what is required, and
// updating is a partial change: every field here is a `std::optional`, so only
// what the caller set goes on the wire and an unmentioned field is left alone
// rather than cleared. A plain QString would have made "leave the description"
// and "remove the description" the same request, and a plain QStringList would
// have made "leave the permissions" indistinguishable from "revoke all of them"
// — which on this type is the difference between a role that works and a role
// that grants nothing.
//
// Creation still needs `roleName` and `permissions`; the server rejects a
// request without them. They are optional here because the same object has to
// serve the update that changes only a description.
//
// **The field is `role_name` going out and `name` coming back.** That is the
// API's spelling, not a slip: Core::OrganizationRole reads `name`. The setter
// is named for the wire so the mismatch is visible at the one place it exists
// rather than hidden behind a `setName()` that writes something else.
class QTOPENAI_CORE_EXPORT RoleRequest
{
public:
    RoleRequest();
    RoleRequest(const RoleRequest &other);
    RoleRequest(RoleRequest &&other) noexcept;
    RoleRequest &operator=(const RoleRequest &other);
    RoleRequest &operator=(RoleRequest &&other) noexcept;
    ~RoleRequest();

    void swap(RoleRequest &other) noexcept { d.swap(other.d); }

    std::optional<QString> roleName() const;
    void setRoleName(const QString &roleName);

    std::optional<QStringList> permissions() const;
    void setPermissions(const QStringList &permissions);

    std::optional<QString> description() const;
    void setDescription(const QString &description);

    // True while nothing has been set — a request that would ask the server to
    // change nothing.
    bool isEmpty() const;

    QJsonObject toJson() const;
    static RoleRequest fromJson(const QJsonObject &json);

    bool operator==(const RoleRequest &other) const;
    bool operator!=(const RoleRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<RoleRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::RoleRequest)
Q_DECLARE_METATYPE(QtOpenAi::Core::RoleRequest)
