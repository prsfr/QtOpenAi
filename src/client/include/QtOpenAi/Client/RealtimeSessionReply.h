// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/RealtimeSessionConfig.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for POST /realtime/sessions, which answers with the
// resolved session configuration itself. See RestReplyBase for the shared
// lifecycle.
class QTOPENAI_CLIENT_EXPORT RealtimeSessionReply : public TypedReply<Core::RealtimeSessionConfig>
{
    Q_OBJECT
public:
    Core::RealtimeSessionConfig session() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::RealtimeSessionConfig &session);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::RealtimeSessionConfig &session) override
    {
        Q_EMIT finished(session);
    }
};

} // namespace Client
} // namespace QtOpenAi
