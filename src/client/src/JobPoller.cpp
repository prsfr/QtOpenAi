// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/JobPoller.h"

#include "QtOpenAi/Client/Client.h"

#include "JobPoller_p.h"

#include <QtCore/QTimer>

namespace QtOpenAi {
namespace Client {

JobPoller::JobPoller(JobPollerPrivate &dd, Client *client, QString jobId, int intervalMs,
                     QObject *parent)
    : QObject(parent)
    , d_ptr(&dd)
{
    Q_D(JobPoller);
    d->client = client;
    d->jobId = std::move(jobId);
    d->intervalMs = intervalMs > 0 ? intervalMs : d->intervalMs;
    d->timer = new QTimer(this);
    d->timer->setSingleShot(true);
    connect(d->timer, &QTimer::timeout, this, [this] {
        Q_D(JobPoller);
        if (d->polling)
            start();
    });
}

JobPoller::~JobPoller() = default;

QString JobPoller::jobId() const
{
    Q_D(const JobPoller);
    return d->jobId;
}

int JobPoller::pollIntervalMs() const
{
    Q_D(const JobPoller);
    return d->intervalMs;
}

void JobPoller::setPollIntervalMs(int intervalMs)
{
    Q_D(JobPoller);
    if (intervalMs > 0)
        d->intervalMs = intervalMs;
}

bool JobPoller::isPolling() const
{
    Q_D(const JobPoller);
    return d->polling;
}

bool JobPoller::isFinished() const
{
    Q_D(const JobPoller);
    return d->finished;
}

void JobPoller::setAutoDelete(bool enabled)
{
    Q_D(JobPoller);
    d->autoDelete = enabled;
}

bool JobPoller::autoDelete() const
{
    Q_D(const JobPoller);
    return d->autoDelete;
}

Client *JobPoller::client() const
{
    Q_D(const JobPoller);
    return d->client;
}

void JobPoller::start()
{
    Q_D(JobPoller);
    if (d->finished)
        return;
    d->polling = true;
    d->timer->stop();

    if (!d->client) {
        finish();
        Q_EMIT failed(ClientError(ClientError::Kind::Network,
                                  QStringLiteral("client no longer available"), 0));
        return;
    }

    requestPoll();
}

void JobPoller::stop()
{
    Q_D(JobPoller);
    d->polling = false;
    d->timer->stop();
}

void JobPoller::finish()
{
    Q_D(JobPoller);
    d->polling = false;
    d->finished = true;
    if (d->autoDelete)
        deleteLater();
}

void JobPoller::scheduleNextPoll()
{
    Q_D(JobPoller);
    d->timer->start(d->intervalMs);
}

} // namespace Client
} // namespace QtOpenAi
