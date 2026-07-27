// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Run.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for GET /threads/{id}/runs, returning a
// cursor-paginated page of runs. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT RunListReply : public TypedReply<Core::RunList>
{
    Q_OBJECT
public:
    Core::RunList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::RunList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::RunList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
