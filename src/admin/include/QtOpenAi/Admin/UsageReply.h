// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>
#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/OrganizationUsage.h>

namespace QtOpenAi {

namespace Client {
// Declared here so the friendship below can name it; including Client.h would
// pull ~70 reply headers into every user of this one.
class Client;
} // namespace Client

namespace Admin {

// An asynchronous handle for one page of usage buckets (GET
// /organization/usage/*). See Client::RestReplyBase for the shared lifecycle.
//
// One reply type for all ten usage endpoints, because they all answer with the
// same UsagePage -- see Organization::UsageKind for which endpoint a reply came
// from, and Core::UsageResult for how one row can describe any of them.
class QTOPENAI_ADMIN_EXPORT UsageReply : public Client::TypedReply<Core::UsagePage>
{
    Q_OBJECT
public:
    Core::UsagePage usage() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::UsagePage &usage);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::UsagePage &usage) override { Q_EMIT finished(usage); }
};

} // namespace Admin
} // namespace QtOpenAi
