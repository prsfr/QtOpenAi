// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>
#include <QtOpenAi/Core/CompletionResponse.h>

#include <QtCore/QObject>

#include <functional>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class CompletionReplyPrivate;

// An asynchronous handle for one legacy text-completion request
// (POST /completions). Emits finished() on success and failed() on error; both
// are followed by done(). Auto-deletes after done() unless disabled.
class QTOPENAI_CLIENT_EXPORT CompletionReply : public QObject
{
    Q_OBJECT
public:
    ~CompletionReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    Core::CompletionResponse response() const;
    ClientError error() const;

    RateLimit rateLimit() const;
    int retryCount() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    void finished(const QtOpenAi::Core::CompletionResponse &response);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();
    void retrying(int attempt, int delayMs);

private:
    friend class Client;
    CompletionReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                    QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(CompletionReply)
    QScopedPointer<CompletionReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
