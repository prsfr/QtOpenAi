// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/FineTuningCheckpoint.h>

namespace QtOpenAi {
namespace Client {

class FineTuningPermissionListReplyPrivate;

// An asynchronous handle for the checkpoint-permission list endpoints (GET and
// POST /fine_tuning/checkpoints/{checkpoint}/permissions) — creating grants also
// answers with a list. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT FineTuningPermissionListReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::FineTuningCheckpointPermissionList list() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::FineTuningCheckpointPermissionList &list);

private:
    friend class Client;
    FineTuningPermissionListReply(std::function<QNetworkReply *()> requestFactory,
                                  RetryPolicy policy, QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(FineTuningPermissionListReply)
};

} // namespace Client
} // namespace QtOpenAi
