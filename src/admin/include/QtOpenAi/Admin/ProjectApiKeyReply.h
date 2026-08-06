// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>
#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/ProjectApiKey.h>

namespace QtOpenAi {

namespace Client {
// Declared here so the friendships below can name it; including Client.h would
// pull ~70 reply headers into every user of this one.
class Client;
} // namespace Client

namespace Admin {

// An asynchronous handle for one project API key
// (GET/DELETE /organization/projects/{id}/api_keys/{key_id}). See
// Client::RestReplyBase for the shared lifecycle.
//
// Read-and-revoke only: there is no create endpoint, and the value that comes
// back is redacted — see Core::ProjectApiKey.
class QTOPENAI_ADMIN_EXPORT ProjectApiKeyReply : public Client::TypedReply<Core::ProjectApiKey>
{
    Q_OBJECT
public:
    Core::ProjectApiKey apiKey() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ProjectApiKey &apiKey);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ProjectApiKey &apiKey) override { Q_EMIT finished(apiKey); }
};

// An asynchronous handle for one page of project API keys
// (GET /organization/projects/{id}/api_keys).
class QTOPENAI_ADMIN_EXPORT ProjectApiKeyListReply
    : public Client::TypedReply<Core::ProjectApiKeyList>
{
    Q_OBJECT
public:
    Core::ProjectApiKeyList apiKeys() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ProjectApiKeyList &apiKeys);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ProjectApiKeyList &apiKeys) override { Q_EMIT finished(apiKeys); }
};

} // namespace Admin
} // namespace QtOpenAi
