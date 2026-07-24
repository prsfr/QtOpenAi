// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>
#include <QtOpenAi/Core/VideoJob.h>

#include <QtCore/QObject>

#include <functional>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class VideoReplyPrivate;

// An asynchronous handle for a single-video request (POST /videos, GET
// /videos/{id}, POST /videos/{id}/remix, DELETE /videos/{id}). All return a
// VideoJob shape, so this reply serves them all. Emits finished() on success and
// failed() on error; both are followed by done(). Auto-deletes after done()
// unless disabled.
class QTOPENAI_CLIENT_EXPORT VideoReply : public QObject
{
    Q_OBJECT
public:
    ~VideoReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    Core::VideoJob job() const;
    ClientError error() const;

    RateLimit rateLimit() const;
    int retryCount() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VideoJob &job);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();
    void retrying(int attempt, int delayMs);

private:
    friend class Client;
    VideoReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
               QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(VideoReply)
    QScopedPointer<VideoReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
