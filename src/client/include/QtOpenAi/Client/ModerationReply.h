// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/ModerationResponse.h>

namespace QtOpenAi {
namespace Client {

// A moderation request (POST /moderations).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ModerationReply : public TypedReply<Core::ModerationResponse>
{
    Q_OBJECT
public:
    Core::ModerationResponse response() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ModerationResponse &response);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ModerationResponse &response) override
    {
        Q_EMIT finished(response);
    }
};

} // namespace Client
} // namespace QtOpenAi
