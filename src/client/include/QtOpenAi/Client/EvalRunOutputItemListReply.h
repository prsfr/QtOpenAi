// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/EvalRun.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for GET /evals/{eval_id}/runs/{run_id}/output_items,
// returning a page of graded results. See RestReplyBase for the shared
// lifecycle.
class QTOPENAI_CLIENT_EXPORT EvalRunOutputItemListReply
    : public TypedReply<Core::EvalRunOutputItemList>
{
    Q_OBJECT
public:
    Core::EvalRunOutputItemList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::EvalRunOutputItemList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::EvalRunOutputItemList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
