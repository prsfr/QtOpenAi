// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/ChatCompletionResponse.h>

namespace QtOpenAi {
namespace Client {

// A chat completion (POST /chat/completions) or a stored completion
// retrieved/updated via /chat/completions/{id}.
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ChatCompletionReply : public TypedReply<Core::ChatCompletionResponse>
{
    Q_OBJECT
public:
    Core::ChatCompletionResponse response() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ChatCompletionResponse &response);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ChatCompletionResponse &response) override
    {
        Q_EMIT finished(response);
    }
};

} // namespace Client
} // namespace QtOpenAi
