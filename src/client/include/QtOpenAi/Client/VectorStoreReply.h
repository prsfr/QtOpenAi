// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/VectorStore.h>

namespace QtOpenAi {
namespace Client {

class VectorStoreReplyPrivate;

// An asynchronous handle for a single vector store (POST /vector_stores,
// GET/POST/DELETE /vector_stores/{id}). All of them answer with the store
// shape, so this reply serves them all — including the deletion
// acknowledgement, whose object() is "vector_store.deleted".
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT VectorStoreReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::VectorStore store() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VectorStore &store);

private:
    friend class Client;
    VectorStoreReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                     QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(VectorStoreReply)
};

} // namespace Client
} // namespace QtOpenAi
