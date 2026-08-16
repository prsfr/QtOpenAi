// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>
#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/DataRetention.h>
#include <QtOpenAi/Core/SpendAlert.h>

namespace QtOpenAi {

namespace Client {
// Declared here so the friendships below can name it; including Client.h would
// pull ~70 reply headers into every user of this one.
class Client;
} // namespace Client

namespace Admin {

// An asynchronous handle for one spend alert, at either scope. See
// Client::RestReplyBase for the shared lifecycle.
//
// The reply types of this module derive from the same TypedReply<T> as every
// other endpoint's, and are created through the same request path -- see
// Organization. The friend declaration names Client::Client because that is
// where replies are constructed, so that a reply still cannot be created from
// outside the library.
//
// The deletion acknowledgement decodes into Core::SpendAlert as well, reporting
// the object as "organization.spend_alert.deleted" or the project's equivalent.
class QTOPENAI_ADMIN_EXPORT SpendAlertReply : public Client::TypedReply<Core::SpendAlert>
{
    Q_OBJECT
public:
    Core::SpendAlert alert() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::SpendAlert &alert);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::SpendAlert &alert) override { Q_EMIT finished(alert); }
};

// An asynchronous handle for a page of spend alerts, at either scope.
class QTOPENAI_ADMIN_EXPORT SpendAlertListReply : public Client::TypedReply<Core::SpendAlertList>
{
    Q_OBJECT
public:
    Core::SpendAlertList alerts() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::SpendAlertList &alerts);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::SpendAlertList &alerts) override { Q_EMIT finished(alerts); }
};

// An asynchronous handle for a data-retention setting, at either scope.
class QTOPENAI_ADMIN_EXPORT DataRetentionReply : public Client::TypedReply<Core::DataRetention>
{
    Q_OBJECT
public:
    Core::DataRetention retention() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::DataRetention &retention);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::DataRetention &retention) override { Q_EMIT finished(retention); }
};

} // namespace Admin
} // namespace QtOpenAi
