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

class VideoListReplyPrivate;

// An asynchronous handle for a videos-list request (GET /videos). Emits
// finished() on success and failed() on error; both are followed by done().
// Auto-deletes after done() unless disabled.
class QTOPENAI_CLIENT_EXPORT VideoListReply : public QObject
{
    Q_OBJECT
public:
    ~VideoListReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    Core::VideoList list() const;
    ClientError error() const;

    RateLimit rateLimit() const;
    int retryCount() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VideoList &list);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();
    void retrying(int attempt, int delayMs);

private:
    friend class Client;
    VideoListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                   QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(VideoListReply)
    QScopedPointer<VideoListReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
