// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/TranscriptionResponse.h>

namespace QtOpenAi {
namespace Client {

// A speech-to-text request (POST /audio/transcriptions or /audio/translations).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT TranscriptionReply : public TypedReply<Core::TranscriptionResponse>
{
    Q_OBJECT
public:
    Core::TranscriptionResponse response() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::TranscriptionResponse &response);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::TranscriptionResponse &response) override
    {
        Q_EMIT finished(response);
    }
};

} // namespace Client
} // namespace QtOpenAi
