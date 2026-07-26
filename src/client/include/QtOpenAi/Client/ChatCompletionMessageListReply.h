// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/ChatCompletionList.h>

namespace QtOpenAi {
namespace Client {

// The input messages of a stored chat completion
// (GET /chat/completions/{id}/messages).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ChatCompletionMessageListReply
    : public TypedReply<Core::ChatCompletionMessageList>
{
    Q_OBJECT
public:
    Core::ChatCompletionMessageList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ChatCompletionMessageList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ChatCompletionMessageList &list) override
    {
        Q_EMIT finished(list);
    }
};

} // namespace Client
} // namespace QtOpenAi
