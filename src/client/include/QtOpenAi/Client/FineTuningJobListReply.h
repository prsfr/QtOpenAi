// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/FineTuningJob.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for GET /fine_tuning/jobs, returning a cursor-paginated
// page of fine-tuning jobs. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT FineTuningJobListReply : public TypedReply<Core::FineTuningJobList>
{
    Q_OBJECT
public:
    Core::FineTuningJobList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::FineTuningJobList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::FineTuningJobList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
