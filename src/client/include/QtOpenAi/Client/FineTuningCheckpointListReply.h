// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/FineTuningCheckpoint.h>

namespace QtOpenAi {
namespace Client {

class FineTuningCheckpointListReplyPrivate;

// An asynchronous handle for GET /fine_tuning/jobs/{id}/checkpoints, returning a
// page of mid-training snapshots. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT FineTuningCheckpointListReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::FineTuningJobCheckpointList list() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::FineTuningJobCheckpointList &list);

private:
    friend class Client;
    FineTuningCheckpointListReply(std::function<QNetworkReply *()> requestFactory,
                                  RetryPolicy policy, QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(FineTuningCheckpointListReply)
};

} // namespace Client
} // namespace QtOpenAi
