// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/SpeechReply.h"

#include "RestReply_p.h"

namespace QtOpenAi {
namespace Client {

class SpeechReplyPrivate
{
public:
    RestReply *engine = nullptr;
    QByteArray audioData;
    QByteArray contentType;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

SpeechReply::SpeechReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                         QObject *parent)
    : QObject(parent)
    , d_ptr(new SpeechReplyPrivate)
{
    Q_D(SpeechReply);
    d->engine = new RestReply(std::move(requestFactory), std::move(policy), this);

    connect(d->engine, &RestReply::retrying, this, &SpeechReply::retrying);

    connect(d->engine, &RestReply::succeeded, this, [this](const QByteArray &body, int) {
        Q_D(SpeechReply);
        // Binary payload: surface the bytes verbatim, no JSON parsing.
        d->audioData = body;
        d->contentType = d->engine->contentType();
        d->success = true;
        d->finished = true;
        Q_EMIT finished(d->audioData);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });

    connect(d->engine, &RestReply::failed, this, [this](const ClientError &error) {
        Q_D(SpeechReply);
        d->finished = true;
        d->success = false;
        d->error = error;
        Q_EMIT failed(error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

SpeechReply::~SpeechReply() = default;

bool SpeechReply::isFinished() const
{
    Q_D(const SpeechReply);
    return d->finished;
}

bool SpeechReply::isSuccess() const
{
    Q_D(const SpeechReply);
    return d->success;
}

QByteArray SpeechReply::audioData() const
{
    Q_D(const SpeechReply);
    return d->audioData;
}

QByteArray SpeechReply::contentType() const
{
    Q_D(const SpeechReply);
    return d->contentType;
}

ClientError SpeechReply::error() const
{
    Q_D(const SpeechReply);
    return d->error;
}

RateLimit SpeechReply::rateLimit() const
{
    Q_D(const SpeechReply);
    return d->engine->rateLimit();
}

int SpeechReply::retryCount() const
{
    Q_D(const SpeechReply);
    return d->engine->retryCount();
}

void SpeechReply::setAutoDelete(bool enabled)
{
    Q_D(SpeechReply);
    d->autoDelete = enabled;
}

bool SpeechReply::autoDelete() const
{
    Q_D(const SpeechReply);
    return d->autoDelete;
}

void SpeechReply::abort()
{
    Q_D(SpeechReply);
    d->engine->abort();
}

} // namespace Client
} // namespace QtOpenAi
