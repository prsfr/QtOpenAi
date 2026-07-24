// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VideoContentReply.h"

#include "RestReply_p.h"

namespace QtOpenAi {
namespace Client {

class VideoContentReplyPrivate
{
public:
    RestReply *engine = nullptr;
    QByteArray videoData;
    QByteArray contentType;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

VideoContentReply::VideoContentReply(std::function<QNetworkReply *()> requestFactory,
                                     RetryPolicy policy, QObject *parent)
    : QObject(parent)
    , d_ptr(new VideoContentReplyPrivate)
{
    Q_D(VideoContentReply);
    d->engine = new RestReply(std::move(requestFactory), std::move(policy), this);

    connect(d->engine, &RestReply::retrying, this, &VideoContentReply::retrying);

    connect(d->engine, &RestReply::succeeded, this, [this](const QByteArray &body, int) {
        Q_D(VideoContentReply);
        // Binary payload: surface the bytes verbatim, no JSON parsing.
        d->videoData = body;
        d->contentType = d->engine->contentType();
        d->success = true;
        d->finished = true;
        Q_EMIT finished(d->videoData);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });

    connect(d->engine, &RestReply::failed, this, [this](const ClientError &error) {
        Q_D(VideoContentReply);
        d->finished = true;
        d->success = false;
        d->error = error;
        Q_EMIT failed(error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

VideoContentReply::~VideoContentReply() = default;

bool VideoContentReply::isFinished() const
{
    Q_D(const VideoContentReply);
    return d->finished;
}

bool VideoContentReply::isSuccess() const
{
    Q_D(const VideoContentReply);
    return d->success;
}

QByteArray VideoContentReply::videoData() const
{
    Q_D(const VideoContentReply);
    return d->videoData;
}

QByteArray VideoContentReply::contentType() const
{
    Q_D(const VideoContentReply);
    return d->contentType;
}

ClientError VideoContentReply::error() const
{
    Q_D(const VideoContentReply);
    return d->error;
}

RateLimit VideoContentReply::rateLimit() const
{
    Q_D(const VideoContentReply);
    return d->engine->rateLimit();
}

int VideoContentReply::retryCount() const
{
    Q_D(const VideoContentReply);
    return d->engine->retryCount();
}

void VideoContentReply::setAutoDelete(bool enabled)
{
    Q_D(VideoContentReply);
    d->autoDelete = enabled;
}

bool VideoContentReply::autoDelete() const
{
    Q_D(const VideoContentReply);
    return d->autoDelete;
}

void VideoContentReply::abort()
{
    Q_D(VideoContentReply);
    d->engine->abort();
}

} // namespace Client
} // namespace QtOpenAi
