// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/CompletionResponse.h>

namespace QtOpenAi {
namespace Client {

// A legacy text completion (POST /completions).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT CompletionReply : public TypedReply<Core::CompletionResponse>
{
    Q_OBJECT
public:
    Core::CompletionResponse response() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::CompletionResponse &response);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::CompletionResponse &response) override
    {
        Q_EMIT finished(response);
    }
};

} // namespace Client
} // namespace QtOpenAi
