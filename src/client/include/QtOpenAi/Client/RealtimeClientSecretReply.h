// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/RealtimeClientSecret.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for the endpoints that mint an ephemeral Realtime
// credential: POST /realtime/client_secrets, /realtime/translations/client_secrets
// and the pre-GA /realtime/transcription_sessions, whose nested spelling the
// value type accepts. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT RealtimeClientSecretReply
    : public TypedReply<Core::RealtimeClientSecret>
{
    Q_OBJECT
public:
    Core::RealtimeClientSecret clientSecret() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::RealtimeClientSecret &clientSecret);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::RealtimeClientSecret &clientSecret) override
    {
        Q_EMIT finished(clientSecret);
    }
};

} // namespace Client
} // namespace QtOpenAi
