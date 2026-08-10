// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>
#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/ProjectPermissions.h>

namespace QtOpenAi {

namespace Client {
// Declared here so the friendships below can name it; including Client.h would
// pull ~70 reply headers into every user of this one.
class Client;
} // namespace Client

namespace Admin {

// An asynchronous handle for a project's model policy (GET/POST/DELETE
// /organization/projects/{project_id}/model_permissions). See
// Client::RestReplyBase for the shared lifecycle.
//
// The reply types of this module derive from the same TypedReply<T> as every
// other endpoint's, and are created through the same request path -- see
// Organization. The friend declaration names Client::Client because that is
// where replies are constructed, so that a reply still cannot be created from
// outside the library.
//
// The deletion acknowledgement decodes into Core::ProjectModelPermissions as
// well, reporting the object as "project.model_permissions.deleted".
class QTOPENAI_ADMIN_EXPORT ProjectModelPermissionsReply
    : public Client::TypedReply<Core::ProjectModelPermissions>
{
    Q_OBJECT
public:
    Core::ProjectModelPermissions permissions() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ProjectModelPermissions &permissions);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ProjectModelPermissions &permissions) override
    {
        Q_EMIT finished(permissions);
    }
};

// An asynchronous handle for a project's hosted-tool switches (GET/POST
// /organization/projects/{project_id}/hosted_tool_permissions).
class QTOPENAI_ADMIN_EXPORT ProjectHostedToolPermissionsReply
    : public Client::TypedReply<Core::ProjectHostedToolPermissions>
{
    Q_OBJECT
public:
    Core::ProjectHostedToolPermissions permissions() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ProjectHostedToolPermissions &permissions);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ProjectHostedToolPermissions &permissions) override
    {
        Q_EMIT finished(permissions);
    }
};

} // namespace Admin
} // namespace QtOpenAi
