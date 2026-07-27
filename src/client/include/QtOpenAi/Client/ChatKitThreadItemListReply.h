// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/ChatKitThreadItem.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for GET /chatkit/threads/{id}/items, returning a
// cursor-paginated page of thread items. See RestReplyBase for the shared
// lifecycle.
class QTOPENAI_CLIENT_EXPORT ChatKitThreadItemListReply
    : public TypedReply<Core::ChatKitThreadItemList>
{
    Q_OBJECT
public:
    Core::ChatKitThreadItemList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ChatKitThreadItemList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ChatKitThreadItemList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
