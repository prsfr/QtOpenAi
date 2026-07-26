// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/ChatCompletionList.h>

namespace QtOpenAi {
namespace Client {

// A list of stored chat completions (GET /chat/completions).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ChatCompletionListReply : public TypedReply<Core::ChatCompletionList>
{
    Q_OBJECT
public:
    Core::ChatCompletionList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ChatCompletionList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ChatCompletionList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
