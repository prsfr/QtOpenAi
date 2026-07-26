// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Batch.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for GET /batches, returning a cursor-paginated page of
// batch jobs. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT BatchListReply : public TypedReply<Core::BatchList>
{
    Q_OBJECT
public:
    Core::BatchList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::BatchList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::BatchList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
