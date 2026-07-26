// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/EvalRun.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single eval run (POST/GET/DELETE below
// /evals/{eval_id}/runs). Cancelling is a POST to the run itself, so it shares
// this reply too. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT EvalRunReply : public TypedReply<Core::EvalRun>
{
    Q_OBJECT
public:
    Core::EvalRun run() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::EvalRun &run);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::EvalRun &run) override { Q_EMIT finished(run); }
};

} // namespace Client
} // namespace QtOpenAi
