// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>
#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/SpendLimit.h>

namespace QtOpenAi {

namespace Client {
// Declared here so the friendship below can name it; including Client.h would
// pull ~70 reply headers into every user of this one.
class Client;
} // namespace Client

namespace Admin {

// An asynchronous handle for a hard spend limit, at either scope. See
// Client::RestReplyBase for the shared lifecycle.
//
// The reply types of this module derive from the same TypedReply<T> as every
// other endpoint's, and are created through the same request path -- see
// Organization. The friend declaration names Client::Client because that is
// where replies are constructed, so that a reply still cannot be created from
// outside the library.
//
// There is no list reply because there is no list endpoint: a scope has one
// limit or none. The deletion acknowledgement decodes into Core::SpendLimit as
// well, reporting the object as "organization.spend_limit.deleted" or the
// project's equivalent.
class QTOPENAI_ADMIN_EXPORT SpendLimitReply : public Client::TypedReply<Core::SpendLimit>
{
    Q_OBJECT
public:
    Core::SpendLimit limit() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::SpendLimit &limit);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::SpendLimit &limit) override { Q_EMIT finished(limit); }
};

} // namespace Admin
} // namespace QtOpenAi
