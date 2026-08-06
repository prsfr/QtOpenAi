// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>
#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Project.h>

namespace QtOpenAi {

namespace Client {
// Declared here so the friendships below can name it; including Client.h would
// pull ~70 reply headers into every user of this one.
class Client;
} // namespace Client

namespace Admin {

// An asynchronous handle for one project (GET/POST
// /organization/projects/{project_id}, POST .../archive). See
// Client::RestReplyBase for the shared lifecycle.
//
// The reply types of this module derive from the same TypedReply<T> as every
// other endpoint's, and are created through the same request path -- see
// Organization. The friend declaration names Client::Client because that is
// where replies are constructed, so that a reply still cannot be created from
// outside the library.
//
// Archiving answers with the project too, its status changed -- there is no
// deletion acknowledgement here because there is no DELETE. See Core::Project.
class QTOPENAI_ADMIN_EXPORT ProjectReply : public Client::TypedReply<Core::Project>
{
    Q_OBJECT
public:
    Core::Project project() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Project &project);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::Project &project) override { Q_EMIT finished(project); }
};

// An asynchronous handle for one page of projects (GET /organization/projects).
class QTOPENAI_ADMIN_EXPORT ProjectListReply : public Client::TypedReply<Core::ProjectList>
{
    Q_OBJECT
public:
    Core::ProjectList projects() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ProjectList &projects);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ProjectList &projects) override { Q_EMIT finished(projects); }
};

} // namespace Admin
} // namespace QtOpenAi
