// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/Batch.h>

namespace QtOpenAi {
namespace Client {

class BatchReplyPrivate;

// An asynchronous handle for a single-batch request (POST /batches, GET
// /batches/{id}, POST /batches/{id}/cancel). All return a Batch shape, so this
// reply serves them all. See RestReplyBase for the shared lifecycle
// (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT BatchReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::Batch batch() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Batch &batch);

private:
    friend class Client;
    BatchReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
               QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(BatchReply)
};

} // namespace Client
} // namespace QtOpenAi
