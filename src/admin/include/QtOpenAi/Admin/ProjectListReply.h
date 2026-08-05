// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>
#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Project.h>

namespace QtOpenAi {

namespace Client {
// Declared here so the friendship below can name it; including Client.h would
// pull ~70 reply headers into every user of this one.
class Client;
} // namespace Client

namespace Admin {

// An asynchronous handle for one page of projects (GET /organization/projects).
// See Client::RestReplyBase for the shared lifecycle.
//
// The reply types of this module derive from the same TypedReply<T> as every
// other endpoint's, and are created through the same request path -- see
// Organization. The friend declaration names Client::Client because that is
// where replies are constructed, so that a reply still cannot be created from
// outside the library.
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
