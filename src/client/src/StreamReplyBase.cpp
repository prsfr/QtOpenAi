// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/StreamReplyBase.h"

#include "HttpSupport_p.h"
#include "StreamReplyBase_p.h"

#include <QtNetwork/QNetworkReply>

namespace QtOpenAi {
namespace Client {

StreamReplyBase::StreamReplyBase(StreamReplyBasePrivate &dd, QNetworkReply *reply, QObject *parent)
    : QObject(parent)
    , d_ptr(&dd)
{
    Q_D(StreamReplyBase);
    d->networkReply = reply;
    reply->setParent(this);

    connect(reply, &QNetworkReply::readyRead, this, [this]() {
        Q_D(StreamReplyBase);
        const QList<detail::SseEvent> events = d->parser.feed(d->networkReply->readAll());
        for (const detail::SseEvent &sse : events) {
            if (sse.data == "[DONE]")
                continue;
            handleEvent(sse.name, sse.data);
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this]() {
        Q_D(StreamReplyBase);
        d->finished = true;
        QNetworkReply *reply = d->networkReply;
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        d->rateLimit = detail::parseRateLimit(reply);

        if (reply->error() != QNetworkReply::NoError || status >= 400) {
            // An error response is delivered as a single JSON body rather than
            // as events, so whatever the parser is still holding is the start
            // of it rather than half an event.
            d->success = false;
            d->error = detail::errorFromBody(d->parser.buffered() + reply->readAll(),
                                             reply->errorString(), status);
            Q_EMIT failed(d->error);
        } else {
            // isSuccess() is observably true while the subclass emits its
            // finished(...), as it has always been -- and goes back to false if
            // the subclass says what arrived was not a complete answer.
            d->success = true;
            if (!dispatchFinished(status)) {
                d->success = false;
                Q_EMIT failed(d->error);
            }
        }

        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

StreamReplyBase::~StreamReplyBase() = default;

bool StreamReplyBase::isFinished() const
{
    Q_D(const StreamReplyBase);
    return d->finished;
}

bool StreamReplyBase::isSuccess() const
{
    Q_D(const StreamReplyBase);
    return d->success;
}

ClientError StreamReplyBase::error() const
{
    Q_D(const StreamReplyBase);
    return d->error;
}

RateLimit StreamReplyBase::rateLimit() const
{
    Q_D(const StreamReplyBase);
    return d->rateLimit;
}

void StreamReplyBase::setAutoDelete(bool enabled)
{
    Q_D(StreamReplyBase);
    d->autoDelete = enabled;
}

bool StreamReplyBase::autoDelete() const
{
    Q_D(const StreamReplyBase);
    return d->autoDelete;
}

void StreamReplyBase::abort()
{
    Q_D(StreamReplyBase);
    if (d->networkReply && d->networkReply->isRunning())
        d->networkReply->abort();
}

void StreamReplyBase::setError(const ClientError &error)
{
    Q_D(StreamReplyBase);
    d->error = error;
}

} // namespace Client
} // namespace QtOpenAi
