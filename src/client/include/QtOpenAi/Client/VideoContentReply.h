// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>

#include <QtCore/QByteArray>
#include <QtCore/QObject>

#include <functional>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class VideoContentReplyPrivate;

// An asynchronous handle for a rendered-video download (GET /videos/{id}/content).
// The endpoint returns a binary video blob, so the successful payload is exposed
// verbatim as raw bytes (with the response's Content-Type) rather than parsed as
// JSON, mirroring SpeechReply. Emits finished() on success and failed() on error;
// both are followed by done(). Auto-deletes after done() unless disabled.
class QTOPENAI_CLIENT_EXPORT VideoContentReply : public QObject
{
    Q_OBJECT
public:
    ~VideoContentReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    // The raw video bytes returned by the server (empty until finished).
    QByteArray videoData() const;
    // The response Content-Type, e.g. "video/mp4".
    QByteArray contentType() const;
    ClientError error() const;

    RateLimit rateLimit() const;
    int retryCount() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    void finished(const QByteArray &videoData);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();
    void retrying(int attempt, int delayMs);

private:
    friend class Client;
    VideoContentReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                      QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(VideoContentReply)
    QScopedPointer<VideoContentReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
