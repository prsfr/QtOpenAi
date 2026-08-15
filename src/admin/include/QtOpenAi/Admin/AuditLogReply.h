// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>
#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/AuditLog.h>

namespace QtOpenAi {

namespace Client {
// Declared here so the friendship below can name it; including Client.h would
// pull ~70 reply headers into every user of this one.
class Client;
} // namespace Client

namespace Admin {

// An asynchronous handle for a page of audit-log entries (GET
// /organization/audit_logs). See Client::RestReplyBase for the shared lifecycle.
//
// The reply types of this module derive from the same TypedReply<T> as every
// other endpoint's, and are created through the same request path -- see
// Organization. The friend declaration names Client::Client because that is
// where replies are constructed, so that a reply still cannot be created from
// outside the library.
//
// There is no single-entry reply because there is no single-entry endpoint: the
// audit trail is only ever read a page at a time.
class QTOPENAI_ADMIN_EXPORT AuditLogListReply : public Client::TypedReply<Core::AuditLogList>
{
    Q_OBJECT
public:
    Core::AuditLogList auditLogs() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::AuditLogList &auditLogs);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::AuditLogList &auditLogs) override { Q_EMIT finished(auditLogs); }
};

} // namespace Admin
} // namespace QtOpenAi
