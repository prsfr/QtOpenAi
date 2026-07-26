// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Conversation.h>

namespace QtOpenAi {
namespace Client {

// A conversation resource (/conversations); the reply also carries the
// acknowledgement returned by delete operations.
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ConversationReply : public TypedReply<Core::Conversation>
{
    Q_OBJECT
public:
    Core::Conversation conversation() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Conversation &conversation);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::Conversation &conversation) override
    {
        Q_EMIT finished(conversation);
    }
};

} // namespace Client
} // namespace QtOpenAi
