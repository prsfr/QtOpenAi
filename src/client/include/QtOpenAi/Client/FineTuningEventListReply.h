// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/FineTuningJob.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for GET /fine_tuning/jobs/{id}/events, returning a page
// of the job's progress log. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT FineTuningEventListReply
    : public TypedReply<Core::FineTuningJobEventList>
{
    Q_OBJECT
public:
    Core::FineTuningJobEventList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::FineTuningJobEventList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::FineTuningJobEventList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
