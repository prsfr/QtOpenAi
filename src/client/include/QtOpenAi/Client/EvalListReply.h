// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Eval.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for GET /evals, returning a cursor-paginated page of
// eval definitions. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT EvalListReply : public TypedReply<Core::EvalList>
{
    Q_OBJECT
public:
    Core::EvalList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::EvalList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::EvalList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
