// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/FineTuningCheckpoint.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for DELETE
// /fine_tuning/checkpoints/{checkpoint}/permissions/{id}, whose acknowledgement
// shares the permission shape (object "checkpoint.permission.deleted"). See
// RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT FineTuningPermissionReply
    : public TypedReply<Core::FineTuningCheckpointPermission>
{
    Q_OBJECT
public:
    Core::FineTuningCheckpointPermission permission() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::FineTuningCheckpointPermission &permission);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::FineTuningCheckpointPermission &permission) override
    {
        Q_EMIT finished(permission);
    }
};

} // namespace Client
} // namespace QtOpenAi
