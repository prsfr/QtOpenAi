// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/TranscriptionReply.h"

#include "RestReply_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

class TranscriptionReplyPrivate
{
public:
    RestReply *engine = nullptr;
    Core::TranscriptionResponse response;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

TranscriptionReply::TranscriptionReply(std::function<QNetworkReply *()> requestFactory,
                                       RetryPolicy policy, QObject *parent)
    : QObject(parent)
    , d_ptr(new TranscriptionReplyPrivate)
{
    Q_D(TranscriptionReply);
    d->engine = new RestReply(std::move(requestFactory), std::move(policy), this);

    connect(d->engine, &RestReply::retrying, this, &TranscriptionReply::retrying);

    connect(d->engine, &RestReply::succeeded, this, [this](const QByteArray &body, int) {
        Q_D(TranscriptionReply);
        // The response_format governs the payload: json/verbose_json return a
        // JSON object; text/srt/vtt return the transcript as plain text.
        const QByteArray trimmed = body.trimmed();
        if (trimmed.startsWith('{')) {
            const QJsonDocument doc = QJsonDocument::fromJson(trimmed);
            d->response = Core::TranscriptionResponse::fromJson(doc.object());
        } else {
            d->response = Core::TranscriptionResponse::fromText(QString::fromUtf8(body));
        }
        d->success = true;
        d->finished = true;
        Q_EMIT finished(d->response);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });

    connect(d->engine, &RestReply::failed, this, [this](const ClientError &error) {
        Q_D(TranscriptionReply);
        d->finished = true;
        d->success = false;
        d->error = error;
        Q_EMIT failed(error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

TranscriptionReply::~TranscriptionReply() = default;

bool TranscriptionReply::isFinished() const
{
    Q_D(const TranscriptionReply);
    return d->finished;
}

bool TranscriptionReply::isSuccess() const
{
    Q_D(const TranscriptionReply);
    return d->success;
}

Core::TranscriptionResponse TranscriptionReply::response() const
{
    Q_D(const TranscriptionReply);
    return d->response;
}

ClientError TranscriptionReply::error() const
{
    Q_D(const TranscriptionReply);
    return d->error;
}

RateLimit TranscriptionReply::rateLimit() const
{
    Q_D(const TranscriptionReply);
    return d->engine->rateLimit();
}

int TranscriptionReply::retryCount() const
{
    Q_D(const TranscriptionReply);
    return d->engine->retryCount();
}

void TranscriptionReply::setAutoDelete(bool enabled)
{
    Q_D(TranscriptionReply);
    d->autoDelete = enabled;
}

bool TranscriptionReply::autoDelete() const
{
    Q_D(const TranscriptionReply);
    return d->autoDelete;
}

void TranscriptionReply::abort()
{
    Q_D(TranscriptionReply);
    d->engine->abort();
}

} // namespace Client
} // namespace QtOpenAi
