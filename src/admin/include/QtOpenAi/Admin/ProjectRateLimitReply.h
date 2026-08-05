// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>
#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/ProjectRateLimit.h>

namespace QtOpenAi {

namespace Client {
// Declared here so the friendships below can name it; including Client.h would
// pull ~70 reply headers into every user of this one.
class Client;
} // namespace Client

namespace Admin {

// An asynchronous handle for one project rate limit
// (POST /organization/projects/{id}/rate_limits/{rate_limit_id}). See
// Client::RestReplyBase for the shared lifecycle.
class QTOPENAI_ADMIN_EXPORT ProjectRateLimitReply
    : public Client::TypedReply<Core::ProjectRateLimit>
{
    Q_OBJECT
public:
    Core::ProjectRateLimit rateLimit() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ProjectRateLimit &rateLimit);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ProjectRateLimit &rateLimit) override
    {
        Q_EMIT finished(rateLimit);
    }
};

// An asynchronous handle for one page of a project's rate limits
// (GET /organization/projects/{id}/rate_limits).
class QTOPENAI_ADMIN_EXPORT ProjectRateLimitListReply
    : public Client::TypedReply<Core::ProjectRateLimitList>
{
    Q_OBJECT
public:
    Core::ProjectRateLimitList rateLimits() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ProjectRateLimitList &rateLimits);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ProjectRateLimitList &rateLimits) override
    {
        Q_EMIT finished(rateLimits);
    }
};

} // namespace Admin
} // namespace QtOpenAi
