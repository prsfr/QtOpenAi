// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/FineTuningJob.h>

namespace QtOpenAi {
namespace Client {

class FineTuningJobReplyPrivate;

// An asynchronous handle for a single fine-tuning job (POST /fine_tuning/jobs,
// GET /fine_tuning/jobs/{id}, and the cancel/pause/resume actions). All return
// a job shape, so this reply serves them all. See RestReplyBase for the shared
// lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT FineTuningJobReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::FineTuningJob job() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::FineTuningJob &job);

private:
    friend class Client;
    FineTuningJobReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                       QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(FineTuningJobReply)
};

} // namespace Client
} // namespace QtOpenAi
