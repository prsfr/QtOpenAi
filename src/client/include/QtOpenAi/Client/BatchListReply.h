// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/Batch.h>

namespace QtOpenAi {
namespace Client {

class BatchListReplyPrivate;

// An asynchronous handle for GET /batches, returning a cursor-paginated page of
// batch jobs. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT BatchListReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::BatchList list() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::BatchList &list);

private:
    friend class Client;
    BatchListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                   QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(BatchListReply)
};

} // namespace Client
} // namespace QtOpenAi
