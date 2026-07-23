// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>
#include <QtOpenAi/Core/EmbeddingResponse.h>

#include <QtCore/QObject>

#include <functional>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class EmbeddingReplyPrivate;

// An asynchronous handle for one embeddings request (POST /embeddings). Emits
// finished() on success and failed() on error; both are followed by done().
// Auto-deletes after done() unless disabled.
class QTOPENAI_CLIENT_EXPORT EmbeddingReply : public QObject
{
    Q_OBJECT
public:
    ~EmbeddingReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    Core::EmbeddingResponse response() const;
    ClientError error() const;

    RateLimit rateLimit() const;
    int retryCount() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    void finished(const QtOpenAi::Core::EmbeddingResponse &response);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();
    void retrying(int attempt, int delayMs);

private:
    friend class Client;
    EmbeddingReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                   QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(EmbeddingReply)
    QScopedPointer<EmbeddingReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
