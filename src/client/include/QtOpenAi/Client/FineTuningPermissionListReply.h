// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/FineTuningCheckpoint.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for the checkpoint-permission list endpoints (GET and
// POST /fine_tuning/checkpoints/{checkpoint}/permissions) — creating grants also
// answers with a list. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT FineTuningPermissionListReply
    : public TypedReply<Core::FineTuningCheckpointPermissionList>
{
    Q_OBJECT
public:
    Core::FineTuningCheckpointPermissionList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::FineTuningCheckpointPermissionList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::FineTuningCheckpointPermissionList &list) override
    {
        Q_EMIT finished(list);
    }
};

} // namespace Client
} // namespace QtOpenAi
