// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Voice.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single voice consent (POST/GET/DELETE below
// /audio/voice_consents). All of them return a consent shape — the deletion
// acknowledgement included — so this reply serves them all. See RestReplyBase
// for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT VoiceConsentReply : public TypedReply<Core::VoiceConsent>
{
    Q_OBJECT
public:
    Core::VoiceConsent consent() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VoiceConsent &consent);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::VoiceConsent &consent) override { Q_EMIT finished(consent); }
};

} // namespace Client
} // namespace QtOpenAi
