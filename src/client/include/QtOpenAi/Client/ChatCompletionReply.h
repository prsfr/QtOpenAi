// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>
#include <QtOpenAi/Core/ChatCompletionResponse.h>

#include <QtCore/QObject>

#include <functional>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class ChatCompletionReplyPrivate;

// An asynchronous handle for one chat-completion request. Emits finished() on
// success and failed() on error; both are followed by done(). The object
// deletes itself after done() is emitted unless setAutoDelete(false) was set.
class QTOPENAI_CLIENT_EXPORT ChatCompletionReply : public QObject
{
    Q_OBJECT
public:
    ~ChatCompletionReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    Core::ChatCompletionResponse response() const;
    ClientError error() const;

    // Rate-limit information from the last response's headers.
    RateLimit rateLimit() const;

    // Number of retries performed (0 if the first attempt succeeded/failed).
    int retryCount() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    // Abort the in-flight network request, if any.
    void abort();

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ChatCompletionResponse &response);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();
    // Emitted before each scheduled retry, with the 1-based attempt number and
    // the delay (ms) until it fires.
    void retrying(int attempt, int delayMs);

private:
    friend class Client;
    // Constructed with a factory that (re)starts the underlying network request,
    // so the reply can transparently retry per the supplied policy.
    ChatCompletionReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                        QObject *parent = nullptr);

    // (Re)issue the underlying request; drives both the first attempt and retries.
    void start();

    Q_DECLARE_PRIVATE(ChatCompletionReply)
    QScopedPointer<ChatCompletionReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
