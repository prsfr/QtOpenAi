// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/VideoCharacter.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a video character (POST /videos/characters,
// GET /videos/characters/{id}). Unlike the rest of the video surface this is
// not a job: a character is created synchronously and has no status to poll.
// See RestReplyBase for the shared lifecycle (finished/failed/done,
// auto-delete).
class QTOPENAI_CLIENT_EXPORT VideoCharacterReply : public TypedReply<Core::VideoCharacter>
{
    Q_OBJECT
public:
    Core::VideoCharacter character() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VideoCharacter &character);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::VideoCharacter &character) override
    {
        Q_EMIT finished(character);
    }
};

} // namespace Client
} // namespace QtOpenAi
