// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/FineTuningCheckpoint.h>

namespace QtOpenAi {
namespace Client {

class FineTuningPermissionReplyPrivate;

// An asynchronous handle for DELETE
// /fine_tuning/checkpoints/{checkpoint}/permissions/{id}, whose acknowledgement
// shares the permission shape (object "checkpoint.permission.deleted"). See
// RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT FineTuningPermissionReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::FineTuningCheckpointPermission permission() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::FineTuningCheckpointPermission &permission);

private:
    friend class Client;
    FineTuningPermissionReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                              QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(FineTuningPermissionReply)
};

} // namespace Client
} // namespace QtOpenAi
