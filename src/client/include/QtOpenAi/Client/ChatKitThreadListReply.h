// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/ChatKitThread.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for GET /chatkit/threads, returning a cursor-paginated
// page of ChatKit threads. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT ChatKitThreadListReply : public TypedReply<Core::ChatKitThreadList>
{
    Q_OBJECT
public:
    Core::ChatKitThreadList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ChatKitThreadList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ChatKitThreadList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
