// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Eval.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single eval (POST /evals, GET/POST/DELETE
// /evals/{id}). All four return an eval shape — the delete acknowledgement
// included — so this reply serves them all. See RestReplyBase for the shared
// lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT EvalReply : public TypedReply<Core::Eval>
{
    Q_OBJECT
public:
    Core::Eval eval() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Eval &eval);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::Eval &eval) override { Q_EMIT finished(eval); }
};

} // namespace Client
} // namespace QtOpenAi
