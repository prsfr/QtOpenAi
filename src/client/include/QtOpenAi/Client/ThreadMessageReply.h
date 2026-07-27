// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/ThreadMessage.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single message in a thread (POST/GET/DELETE
// around /threads/{id}/messages). See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT ThreadMessageReply : public TypedReply<Core::ThreadMessage>
{
    Q_OBJECT
public:
    Core::ThreadMessage message() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ThreadMessage &message);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ThreadMessage &message) override { Q_EMIT finished(message); }
};

} // namespace Client
} // namespace QtOpenAi
