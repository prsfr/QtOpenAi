// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>
#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/OrganizationCosts.h>

namespace QtOpenAi {

namespace Client {
// Declared here so the friendship below can name it; including Client.h would
// pull ~70 reply headers into every user of this one.
class Client;
} // namespace Client

namespace Admin {

// An asynchronous handle for one page of cost buckets (GET /organization/costs).
// See Client::RestReplyBase for the shared lifecycle.
class QTOPENAI_ADMIN_EXPORT CostsReply : public Client::TypedReply<Core::CostPage>
{
    Q_OBJECT
public:
    Core::CostPage costs() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::CostPage &costs);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::CostPage &costs) override { Q_EMIT finished(costs); }
};

} // namespace Admin
} // namespace QtOpenAi
