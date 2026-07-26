// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/EvalRun.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for GET /evals/{eval_id}/runs, returning a page of an
// eval's runs. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT EvalRunListReply : public TypedReply<Core::EvalRunList>
{
    Q_OBJECT
public:
    Core::EvalRunList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::EvalRunList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::EvalRunList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
