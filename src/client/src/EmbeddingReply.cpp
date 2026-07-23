// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/EmbeddingReply.h"

#include "RestReply_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

class EmbeddingReplyPrivate
{
public:
    RestReply *engine = nullptr;
    Core::EmbeddingResponse response;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

EmbeddingReply::EmbeddingReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                               QObject *parent)
    : QObject(parent)
    , d_ptr(new EmbeddingReplyPrivate)
{
    Q_D(EmbeddingReply);
    d->engine = new RestReply(std::move(requestFactory), std::move(policy), this);

    connect(d->engine, &RestReply::retrying, this, &EmbeddingReply::retrying);

    connect(d->engine, &RestReply::succeeded, this, [this](const QByteArray &body, int status) {
        Q_D(EmbeddingReply);
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            d->success = false;
            d->error = ClientError(
                    ClientError::Kind::Parse,
                    QStringLiteral("invalid JSON response: %1").arg(parseError.errorString()),
                    status);
        } else {
            d->response = Core::EmbeddingResponse::fromJson(doc.object());
            d->success = true;
        }
        d->finished = true;
        if (d->success)
            Q_EMIT finished(d->response);
        else
            Q_EMIT failed(d->error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });

    connect(d->engine, &RestReply::failed, this, [this](const ClientError &error) {
        Q_D(EmbeddingReply);
        d->finished = true;
        d->success = false;
        d->error = error;
        Q_EMIT failed(error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

EmbeddingReply::~EmbeddingReply() = default;

bool EmbeddingReply::isFinished() const
{
    Q_D(const EmbeddingReply);
    return d->finished;
}

bool EmbeddingReply::isSuccess() const
{
    Q_D(const EmbeddingReply);
    return d->success;
}

Core::EmbeddingResponse EmbeddingReply::response() const
{
    Q_D(const EmbeddingReply);
    return d->response;
}

ClientError EmbeddingReply::error() const
{
    Q_D(const EmbeddingReply);
    return d->error;
}

RateLimit EmbeddingReply::rateLimit() const
{
    Q_D(const EmbeddingReply);
    return d->engine->rateLimit();
}

int EmbeddingReply::retryCount() const
{
    Q_D(const EmbeddingReply);
    return d->engine->retryCount();
}

void EmbeddingReply::setAutoDelete(bool enabled)
{
    Q_D(EmbeddingReply);
    d->autoDelete = enabled;
}

bool EmbeddingReply::autoDelete() const
{
    Q_D(const EmbeddingReply);
    return d->autoDelete;
}

void EmbeddingReply::abort()
{
    Q_D(EmbeddingReply);
    d->engine->abort();
}

} // namespace Client
} // namespace QtOpenAi
