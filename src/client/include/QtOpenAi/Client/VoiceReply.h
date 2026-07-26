// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Voice.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for POST /audio/voices, which builds a custom voice
// from an audio sample. See RestReplyBase for the shared lifecycle
// (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT VoiceReply : public TypedReply<Core::Voice>
{
    Q_OBJECT
public:
    Core::Voice voice() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Voice &voice);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::Voice &voice) override { Q_EMIT finished(voice); }
};

} // namespace Client
} // namespace QtOpenAi
