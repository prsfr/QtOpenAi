// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Assistant.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single assistant (POST/GET/DELETE around
// /assistants). The deletion acknowledgement decodes into the same type, with
// its object reported as "assistant.deleted". See RestReplyBase for the shared
// lifecycle.
class QTOPENAI_CLIENT_EXPORT AssistantReply : public TypedReply<Core::Assistant>
{
    Q_OBJECT
public:
    Core::Assistant assistant() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Assistant &assistant);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::Assistant &assistant) override { Q_EMIT finished(assistant); }
};

} // namespace Client
} // namespace QtOpenAi
