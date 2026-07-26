// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Voice.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for GET /audio/voice_consents, returning a
// cursor-paginated page of consents. See RestReplyBase for the shared
// lifecycle.
class QTOPENAI_CLIENT_EXPORT VoiceConsentListReply : public TypedReply<Core::VoiceConsentList>
{
    Q_OBJECT
public:
    Core::VoiceConsentList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VoiceConsentList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::VoiceConsentList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
