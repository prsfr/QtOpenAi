// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/EmbeddingResponse.h>

namespace QtOpenAi {
namespace Client {

// An embeddings request (POST /embeddings).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT EmbeddingReply : public TypedReply<Core::EmbeddingResponse>
{
    Q_OBJECT
public:
    Core::EmbeddingResponse response() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::EmbeddingResponse &response);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::EmbeddingResponse &response) override
    {
        Q_EMIT finished(response);
    }
};

} // namespace Client
} // namespace QtOpenAi
