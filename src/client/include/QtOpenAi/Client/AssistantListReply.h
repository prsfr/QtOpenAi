// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Assistant.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for GET /assistants, returning a cursor-paginated page
// of assistants. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT AssistantListReply : public TypedReply<Core::AssistantList>
{
    Q_OBJECT
public:
    Core::AssistantList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::AssistantList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::AssistantList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
