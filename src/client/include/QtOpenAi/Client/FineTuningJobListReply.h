// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/FineTuningJob.h>

namespace QtOpenAi {
namespace Client {

class FineTuningJobListReplyPrivate;

// An asynchronous handle for GET /fine_tuning/jobs, returning a cursor-paginated
// page of fine-tuning jobs. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT FineTuningJobListReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::FineTuningJobList list() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::FineTuningJobList &list);

private:
    friend class Client;
    FineTuningJobListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                           QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(FineTuningJobListReply)
};

} // namespace Client
} // namespace QtOpenAi
