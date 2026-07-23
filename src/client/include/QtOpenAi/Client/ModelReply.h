// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>
#include <QtOpenAi/Core/Model.h>

#include <QtCore/QObject>

#include <functional>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class ModelReplyPrivate;

// An asynchronous handle for a single-model request (GET /models/{id}). Emits
// finished() on success and failed() on error; both are followed by done().
// Auto-deletes after done() unless disabled.
class QTOPENAI_CLIENT_EXPORT ModelReply : public QObject
{
    Q_OBJECT
public:
    ~ModelReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    Core::Model model() const;
    ClientError error() const;

    RateLimit rateLimit() const;
    int retryCount() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Model &model);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();
    void retrying(int attempt, int delayMs);

private:
    friend class Client;
    ModelReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
               QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(ModelReply)
    QScopedPointer<ModelReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
