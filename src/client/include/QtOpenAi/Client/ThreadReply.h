// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Thread.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single thread (POST/GET/DELETE around /threads).
// The deletion acknowledgement decodes into the same type, with its object
// reported as "thread.deleted". See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT ThreadReply : public TypedReply<Core::Thread>
{
    Q_OBJECT
public:
    Core::Thread thread() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Thread &thread);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::Thread &thread) override { Q_EMIT finished(thread); }
};

} // namespace Client
} // namespace QtOpenAi
