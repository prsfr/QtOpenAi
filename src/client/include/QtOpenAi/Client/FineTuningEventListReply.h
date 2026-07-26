// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/FineTuningJob.h>

namespace QtOpenAi {
namespace Client {

class FineTuningEventListReplyPrivate;

// An asynchronous handle for GET /fine_tuning/jobs/{id}/events, returning a page
// of the job's progress log. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT FineTuningEventListReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::FineTuningJobEventList list() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::FineTuningJobEventList &list);

private:
    friend class Client;
    FineTuningEventListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                             QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(FineTuningEventListReply)
};

} // namespace Client
} // namespace QtOpenAi
