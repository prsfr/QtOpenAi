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

class ModelListReplyPrivate;

// An asynchronous handle for a models-list request (GET /models). Emits
// finished() on success and failed() on error; both are followed by done().
// Auto-deletes after done() unless disabled.
class QTOPENAI_CLIENT_EXPORT ModelListReply : public QObject
{
    Q_OBJECT
public:
    ~ModelListReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    Core::ModelList models() const;
    ClientError error() const;

    RateLimit rateLimit() const;
    int retryCount() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ModelList &models);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();
    void retrying(int attempt, int delayMs);

private:
    friend class Client;
    ModelListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                   QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(ModelListReply)
    QScopedPointer<ModelListReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
