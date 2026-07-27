// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/RunStep.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for GET /threads/{id}/runs/{run_id}/steps, returning a
// cursor-paginated page of run steps. See RestReplyBase for the shared
// lifecycle.
class QTOPENAI_CLIENT_EXPORT RunStepListReply : public TypedReply<Core::RunStepList>
{
    Q_OBJECT
public:
    Core::RunStepList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::RunStepList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::RunStepList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
