// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/EmbeddingResponse.h>

namespace QtOpenAi {
namespace Client {

class EmbeddingReplyPrivate;

// An embeddings request (POST /embeddings).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT EmbeddingReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::EmbeddingResponse response() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::EmbeddingResponse &response);

private:
    friend class Client;
    EmbeddingReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                   QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(EmbeddingReply)
};

} // namespace Client
} // namespace QtOpenAi
