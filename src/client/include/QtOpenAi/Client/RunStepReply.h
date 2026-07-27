// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/RunStep.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single run step
// (GET /threads/{id}/runs/{run_id}/steps/{step_id}). See RestReplyBase for the
// shared lifecycle.
class QTOPENAI_CLIENT_EXPORT RunStepReply : public TypedReply<Core::RunStep>
{
    Q_OBJECT
public:
    Core::RunStep step() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::RunStep &step);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::RunStep &step) override { Q_EMIT finished(step); }
};

} // namespace Client
} // namespace QtOpenAi
