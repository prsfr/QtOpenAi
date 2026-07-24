// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VideoPoller.h"

#include "QtOpenAi/Client/Client.h"
#include "QtOpenAi/Client/VideoReply.h"

#include <QtCore/QPointer>
#include <QtCore/QTimer>

namespace QtOpenAi {
namespace Client {

class VideoPollerPrivate
{
public:
    QPointer<Client> client;
    QString videoId;
    int intervalMs = 2000;
    bool polling = false;
    bool finished = false;
    bool autoDelete = true;
    Core::VideoJob job;
    QTimer *timer = nullptr; // single-shot; re-armed after each response
};

VideoPoller::VideoPoller(Client *client, QString videoId, int intervalMs, QObject *parent)
    : QObject(parent)
    , d_ptr(new VideoPollerPrivate)
{
    Q_D(VideoPoller);
    d->client = client;
    d->videoId = std::move(videoId);
    d->intervalMs = intervalMs > 0 ? intervalMs : 2000;
    d->timer = new QTimer(this);
    d->timer->setSingleShot(true);
    connect(d->timer, &QTimer::timeout, this, [this] {
        Q_D(VideoPoller);
        if (d->polling)
            start();
    });
}

VideoPoller::~VideoPoller() = default;

QString VideoPoller::videoId() const
{
    Q_D(const VideoPoller);
    return d->videoId;
}

int VideoPoller::pollIntervalMs() const
{
    Q_D(const VideoPoller);
    return d->intervalMs;
}

void VideoPoller::setPollIntervalMs(int intervalMs)
{
    Q_D(VideoPoller);
    if (intervalMs > 0)
        d->intervalMs = intervalMs;
}

bool VideoPoller::isPolling() const
{
    Q_D(const VideoPoller);
    return d->polling;
}

bool VideoPoller::isFinished() const
{
    Q_D(const VideoPoller);
    return d->finished;
}

Core::VideoJob VideoPoller::job() const
{
    Q_D(const VideoPoller);
    return d->job;
}

void VideoPoller::setAutoDelete(bool enabled)
{
    Q_D(VideoPoller);
    d->autoDelete = enabled;
}

bool VideoPoller::autoDelete() const
{
    Q_D(const VideoPoller);
    return d->autoDelete;
}

void VideoPoller::start()
{
    Q_D(VideoPoller);
    if (d->finished)
        return;
    d->polling = true;
    d->timer->stop();

    if (!d->client) {
        d->polling = false;
        d->finished = true;
        Q_EMIT failed(ClientError(ClientError::Kind::Network,
                                  QStringLiteral("client no longer available"), 0));
        if (d->autoDelete)
            deleteLater();
        return;
    }

    VideoReply *reply = d->client->getVideo(d->videoId);
    connect(reply, &VideoReply::finished, this, [this](const Core::VideoJob &job) {
        Q_D(VideoPoller);
        if (!d->polling)
            return;
        d->job = job;
        Q_EMIT progressed(job);
        if (job.isTerminal()) {
            d->polling = false;
            d->finished = true;
            Q_EMIT completed(job);
            if (d->autoDelete)
                deleteLater();
        } else {
            d->timer->start(d->intervalMs);
        }
    });
    connect(reply, &VideoReply::failed, this, [this](const ClientError &error) {
        Q_D(VideoPoller);
        if (!d->polling)
            return;
        d->polling = false;
        d->finished = true;
        Q_EMIT failed(error);
        if (d->autoDelete)
            deleteLater();
    });
}

void VideoPoller::stop()
{
    Q_D(VideoPoller);
    d->polling = false;
    d->timer->stop();
}

} // namespace Client
} // namespace QtOpenAi
