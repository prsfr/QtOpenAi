// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/EvalRun.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for GET
// /evals/{eval_id}/runs/{run_id}/output_items/{id} — one item's graded result.
// See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT EvalRunOutputItemReply : public TypedReply<Core::EvalRunOutputItem>
{
    Q_OBJECT
public:
    Core::EvalRunOutputItem item() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::EvalRunOutputItem &item);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::EvalRunOutputItem &item) override { Q_EMIT finished(item); }
};

} // namespace Client
} // namespace QtOpenAi
