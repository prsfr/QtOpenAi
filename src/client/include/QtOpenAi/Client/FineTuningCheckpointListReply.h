// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/FineTuningCheckpoint.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for GET /fine_tuning/jobs/{id}/checkpoints, returning a
// page of mid-training snapshots. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT FineTuningCheckpointListReply
    : public TypedReply<Core::FineTuningJobCheckpointList>
{
    Q_OBJECT
public:
    Core::FineTuningJobCheckpointList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::FineTuningJobCheckpointList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::FineTuningJobCheckpointList &list) override
    {
        Q_EMIT finished(list);
    }
};

} // namespace Client
} // namespace QtOpenAi
