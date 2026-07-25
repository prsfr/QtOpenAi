// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/VectorStoreFile.h>

namespace QtOpenAi {
namespace Client {

class VectorStoreFileBatchReplyPrivate;

// An asynchronous handle for a vector-store file batch (create, retrieve,
// cancel).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT VectorStoreFileBatchReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::VectorStoreFileBatch batch() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VectorStoreFileBatch &batch);

private:
    friend class Client;
    VectorStoreFileBatchReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                              QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(VectorStoreFileBatchReply)
};

} // namespace Client
} // namespace QtOpenAi
