// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>
#include <QtOpenAi/Core/Response.h>

#include <QtCore/QObject>

#include <functional>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class ResponseReplyPrivate;

// An asynchronous handle for one Responses-API request (create/get/cancel/
// delete). Emits finished() on success and failed() on error; both are followed
// by done(). The object deletes itself after done() is emitted unless
// setAutoDelete(false) was set. Mirrors ChatCompletionReply.
class QTOPENAI_CLIENT_EXPORT ResponseReply : public QObject
{
    Q_OBJECT
public:
    ~ResponseReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    Core::Response response() const;
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
    void finished(const QtOpenAi::Core::Response &response);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();
    // Emitted before each scheduled retry, with the 1-based attempt number and
    // the delay (ms) until it fires.
    void retrying(int attempt, int delayMs);

private:
    friend class Client;
    // Constructed with a factory that (re)starts the underlying network request,
    // so the reply can transparently retry per the supplied policy.
    ResponseReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                  QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(ResponseReply)
    QScopedPointer<ResponseReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
