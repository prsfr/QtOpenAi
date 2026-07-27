// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/ThreadMessage.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for GET /threads/{id}/messages, returning a
// cursor-paginated page of messages. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT ThreadMessageListReply : public TypedReply<Core::ThreadMessageList>
{
    Q_OBJECT
public:
    Core::ThreadMessageList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ThreadMessageList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ThreadMessageList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
