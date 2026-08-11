// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>
#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/AdminApiKey.h>

namespace QtOpenAi {

namespace Client {
// Declared here so the friendships below can name it; including Client.h would
// pull ~70 reply headers into every user of this one.
class Client;
} // namespace Client

namespace Admin {

// An asynchronous handle for one admin API key (GET/POST
// /organization/admin_api_keys, GET/DELETE .../{key_id}). See
// Client::RestReplyBase for the shared lifecycle.
//
// The reply types of this module derive from the same TypedReply<T> as every
// other endpoint's, and are created through the same request path -- see
// Organization. The friend declaration names Client::Client because that is
// where replies are constructed, so that a reply still cannot be created from
// outside the library.
//
// **The reply to a create carries the only copy of the new key's secret.** See
// Core::AdminApiKey::value(): no later read returns it. The deletion
// acknowledgement decodes into the same type, reporting the object as
// "organization.admin_api_key.deleted".
class QTOPENAI_ADMIN_EXPORT AdminApiKeyReply : public Client::TypedReply<Core::AdminApiKey>
{
    Q_OBJECT
public:
    Core::AdminApiKey apiKey() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::AdminApiKey &apiKey);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::AdminApiKey &apiKey) override { Q_EMIT finished(apiKey); }
};

// An asynchronous handle for a page of admin API keys. None of them carries a
// secret -- see Core::AdminApiKey.
class QTOPENAI_ADMIN_EXPORT AdminApiKeyListReply : public Client::TypedReply<Core::AdminApiKeyList>
{
    Q_OBJECT
public:
    Core::AdminApiKeyList apiKeys() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::AdminApiKeyList &apiKeys);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::AdminApiKeyList &apiKeys) override { Q_EMIT finished(apiKeys); }
};

} // namespace Admin
} // namespace QtOpenAi
