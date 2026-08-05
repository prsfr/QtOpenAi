// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>
#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/OrganizationUser.h>

namespace QtOpenAi {

namespace Client {
// Declared here so the friendships below can name it; including Client.h would
// pull ~70 reply headers into every user of this one.
class Client;
} // namespace Client

namespace Admin {

// An asynchronous handle for one organization member
// (GET/POST/DELETE /organization/users/{user_id}). See Client::RestReplyBase for
// the shared lifecycle.
//
// The delete acknowledgement decodes into the same OrganizationUser, so removing
// a member and reading one answer with the same reply type — see that class.
class QTOPENAI_ADMIN_EXPORT UserReply : public Client::TypedReply<Core::OrganizationUser>
{
    Q_OBJECT
public:
    Core::OrganizationUser user() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::OrganizationUser &user);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::OrganizationUser &user) override { Q_EMIT finished(user); }
};

// An asynchronous handle for one page of organization members
// (GET /organization/users).
class QTOPENAI_ADMIN_EXPORT UserListReply : public Client::TypedReply<Core::OrganizationUserList>
{
    Q_OBJECT
public:
    Core::OrganizationUserList users() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::OrganizationUserList &users);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::OrganizationUserList &users) override { Q_EMIT finished(users); }
};

} // namespace Admin
} // namespace QtOpenAi
