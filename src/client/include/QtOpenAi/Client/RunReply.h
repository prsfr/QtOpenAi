// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Run.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single run (POST/GET around /threads/{id}/runs,
// plus /cancel and /submit_tool_outputs, which all answer with the updated run).
// See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT RunReply : public TypedReply<Core::Run>
{
    Q_OBJECT
public:
    Core::Run run() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Run &run);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::Run &run) override { Q_EMIT finished(run); }
};

} // namespace Client
} // namespace QtOpenAi
