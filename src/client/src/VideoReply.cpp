// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VideoReply.h"

#include "RestReply_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

class VideoReplyPrivate
{
public:
    RestReply *engine = nullptr;
    Core::VideoJob job;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

VideoReply::VideoReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                       QObject *parent)
    : QObject(parent)
    , d_ptr(new VideoReplyPrivate)
{
    Q_D(VideoReply);
    d->engine = new RestReply(std::move(requestFactory), std::move(policy), this);

    connect(d->engine, &RestReply::retrying, this, &VideoReply::retrying);

    connect(d->engine, &RestReply::succeeded, this, [this](const QByteArray &body, int status) {
        Q_D(VideoReply);
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            d->success = false;
            d->error = ClientError(
                    ClientError::Kind::Parse,
                    QStringLiteral("invalid JSON response: %1").arg(parseError.errorString()),
                    status);
        } else {
            d->job = Core::VideoJob::fromJson(doc.object());
            d->success = true;
        }
        d->finished = true;
        if (d->success)
            Q_EMIT finished(d->job);
        else
            Q_EMIT failed(d->error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });

    connect(d->engine, &RestReply::failed, this, [this](const ClientError &error) {
        Q_D(VideoReply);
        d->finished = true;
        d->success = false;
        d->error = error;
        Q_EMIT failed(error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

VideoReply::~VideoReply() = default;

bool VideoReply::isFinished() const
{
    Q_D(const VideoReply);
    return d->finished;
}

bool VideoReply::isSuccess() const
{
    Q_D(const VideoReply);
    return d->success;
}

Core::VideoJob VideoReply::job() const
{
    Q_D(const VideoReply);
    return d->job;
}

ClientError VideoReply::error() const
{
    Q_D(const VideoReply);
    return d->error;
}

RateLimit VideoReply::rateLimit() const
{
    Q_D(const VideoReply);
    return d->engine->rateLimit();
}

int VideoReply::retryCount() const
{
    Q_D(const VideoReply);
    return d->engine->retryCount();
}

void VideoReply::setAutoDelete(bool enabled)
{
    Q_D(VideoReply);
    d->autoDelete = enabled;
}

bool VideoReply::autoDelete() const
{
    Q_D(const VideoReply);
    return d->autoDelete;
}

void VideoReply::abort()
{
    Q_D(VideoReply);
    d->engine->abort();
}

} // namespace Client
} // namespace QtOpenAi
