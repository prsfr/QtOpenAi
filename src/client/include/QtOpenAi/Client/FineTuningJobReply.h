// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/FineTuningJob.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single fine-tuning job (POST /fine_tuning/jobs,
// GET /fine_tuning/jobs/{id}, and the cancel/pause/resume actions). All return
// a job shape, so this reply serves them all. See RestReplyBase for the shared
// lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT FineTuningJobReply : public TypedReply<Core::FineTuningJob>
{
    Q_OBJECT
public:
    Core::FineTuningJob job() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::FineTuningJob &job);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::FineTuningJob &job) override { Q_EMIT finished(job); }
};

} // namespace Client
} // namespace QtOpenAi
