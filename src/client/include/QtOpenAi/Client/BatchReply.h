// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Batch.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single-batch request (POST /batches, GET
// /batches/{id}, POST /batches/{id}/cancel). All return a Batch shape, so this
// reply serves them all. See RestReplyBase for the shared lifecycle
// (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT BatchReply : public TypedReply<Core::Batch>
{
    Q_OBJECT
public:
    Core::Batch batch() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Batch &batch);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::Batch &batch) override { Q_EMIT finished(batch); }
};

} // namespace Client
} // namespace QtOpenAi
