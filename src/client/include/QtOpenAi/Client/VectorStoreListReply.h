// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/VectorStore.h>

namespace QtOpenAi {
namespace Client {

// A vector-stores list request (GET /vector_stores).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT VectorStoreListReply : public TypedReply<Core::VectorStoreList>
{
    Q_OBJECT
public:
    Core::VectorStoreList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VectorStoreList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::VectorStoreList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
