// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/VectorStoreFile.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a vector-store file batch (create, retrieve,
// cancel).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT VectorStoreFileBatchReply
    : public TypedReply<Core::VectorStoreFileBatch>
{
    Q_OBJECT
public:
    Core::VectorStoreFileBatch batch() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VectorStoreFileBatch &batch);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::VectorStoreFileBatch &batch) override { Q_EMIT finished(batch); }
};

} // namespace Client
} // namespace QtOpenAi
