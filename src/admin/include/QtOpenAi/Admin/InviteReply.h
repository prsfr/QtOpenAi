// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>
#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Invite.h>

namespace QtOpenAi {

namespace Client {
// Declared here so the friendships below can name it; including Client.h would
// pull ~70 reply headers into every user of this one.
class Client;
} // namespace Client

namespace Admin {

// An asynchronous handle for one invitation (POST /organization/invites,
// GET/DELETE /organization/invites/{invite_id}). See Client::RestReplyBase for
// the shared lifecycle.
//
// The delete acknowledgement decodes into the same Invite — see that class.
class QTOPENAI_ADMIN_EXPORT InviteReply : public Client::TypedReply<Core::Invite>
{
    Q_OBJECT
public:
    Core::Invite invite() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Invite &invite);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::Invite &invite) override { Q_EMIT finished(invite); }
};

// An asynchronous handle for one page of invitations (GET /organization/invites).
class QTOPENAI_ADMIN_EXPORT InviteListReply : public Client::TypedReply<Core::InviteList>
{
    Q_OBJECT
public:
    Core::InviteList invites() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::InviteList &invites);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::InviteList &invites) override { Q_EMIT finished(invites); }
};

} // namespace Admin
} // namespace QtOpenAi
