// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/ChatKitThread.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single ChatKit thread (GET/DELETE
// /chatkit/threads/{id}). The deletion acknowledgement decodes into the same
// type, with its object reported as "chatkit.thread.deleted". See RestReplyBase
// for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT ChatKitThreadReply : public TypedReply<Core::ChatKitThread>
{
    Q_OBJECT
public:
    Core::ChatKitThread thread() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ChatKitThread &thread);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ChatKitThread &thread) override { Q_EMIT finished(thread); }
};

} // namespace Client
} // namespace QtOpenAi
