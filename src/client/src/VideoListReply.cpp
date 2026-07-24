// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VideoListReply.h"

#include "RestReply_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

class VideoListReplyPrivate
{
public:
    RestReply *engine = nullptr;
    Core::VideoList list;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

VideoListReply::VideoListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                               QObject *parent)
    : QObject(parent)
    , d_ptr(new VideoListReplyPrivate)
{
    Q_D(VideoListReply);
    d->engine = new RestReply(std::move(requestFactory), std::move(policy), this);

    connect(d->engine, &RestReply::retrying, this, &VideoListReply::retrying);

    connect(d->engine, &RestReply::succeeded, this, [this](const QByteArray &body, int status) {
        Q_D(VideoListReply);
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            d->success = false;
            d->error = ClientError(
                    ClientError::Kind::Parse,
                    QStringLiteral("invalid JSON response: %1").arg(parseError.errorString()),
                    status);
        } else {
            d->list = Core::VideoList::fromJson(doc.object());
            d->success = true;
        }
        d->finished = true;
        if (d->success)
            Q_EMIT finished(d->list);
        else
            Q_EMIT failed(d->error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });

    connect(d->engine, &RestReply::failed, this, [this](const ClientError &error) {
        Q_D(VideoListReply);
        d->finished = true;
        d->success = false;
        d->error = error;
        Q_EMIT failed(error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

VideoListReply::~VideoListReply() = default;

bool VideoListReply::isFinished() const
{
    Q_D(const VideoListReply);
    return d->finished;
}

bool VideoListReply::isSuccess() const
{
    Q_D(const VideoListReply);
    return d->success;
}

Core::VideoList VideoListReply::list() const
{
    Q_D(const VideoListReply);
    return d->list;
}

ClientError VideoListReply::error() const
{
    Q_D(const VideoListReply);
    return d->error;
}

RateLimit VideoListReply::rateLimit() const
{
    Q_D(const VideoListReply);
    return d->engine->rateLimit();
}

int VideoListReply::retryCount() const
{
    Q_D(const VideoListReply);
    return d->engine->retryCount();
}

void VideoListReply::setAutoDelete(bool enabled)
{
    Q_D(VideoListReply);
    d->autoDelete = enabled;
}

bool VideoListReply::autoDelete() const
{
    Q_D(const VideoListReply);
    return d->autoDelete;
}

void VideoListReply::abort()
{
    Q_D(VideoListReply);
    d->engine->abort();
}

} // namespace Client
} // namespace QtOpenAi
