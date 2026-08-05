// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>
#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/ProjectServiceAccount.h>

namespace QtOpenAi {

namespace Client {
// Declared here so the friendships below can name it; including Client.h would
// pull ~70 reply headers into every user of this one.
class Client;
} // namespace Client

namespace Admin {

// An asynchronous handle for one service account (POST
// /organization/projects/{id}/service_accounts, GET/DELETE .../{account_id}).
// See Client::RestReplyBase for the shared lifecycle.
//
// **The creation reply carries the only copy of the secret** — see
// Core::ServiceAccountApiKey. Read it here or lose it.
class QTOPENAI_ADMIN_EXPORT ProjectServiceAccountReply
    : public Client::TypedReply<Core::ProjectServiceAccount>
{
    Q_OBJECT
public:
    Core::ProjectServiceAccount serviceAccount() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ProjectServiceAccount &serviceAccount);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ProjectServiceAccount &serviceAccount) override
    {
        Q_EMIT finished(serviceAccount);
    }
};

// An asynchronous handle for one page of service accounts
// (GET /organization/projects/{id}/service_accounts).
class QTOPENAI_ADMIN_EXPORT ProjectServiceAccountListReply
    : public Client::TypedReply<Core::ProjectServiceAccountList>
{
    Q_OBJECT
public:
    Core::ProjectServiceAccountList serviceAccounts() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ProjectServiceAccountList &serviceAccounts);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ProjectServiceAccountList &serviceAccounts) override
    {
        Q_EMIT finished(serviceAccounts);
    }
};

} // namespace Admin
} // namespace QtOpenAi
