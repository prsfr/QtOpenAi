// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/RestReplyBase.h"

#include "RestReply_p.h"

#include <QtCore/QJsonDocument>

namespace QtOpenAi {
namespace Client {

class RestReplyBasePrivate
{
public:
    RestReply *engine = nullptr;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

RestReplyBase::RestReplyBase(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                             QObject *parent)
    : QObject(parent)
    , d_ptr(new RestReplyBasePrivate)
{
    Q_D(RestReplyBase);
    d->engine = new RestReply(std::move(requestFactory), std::move(policy), this);

    connect(d->engine, &RestReply::retrying, this, &RestReplyBase::retrying);

    connect(d->engine, &RestReply::succeeded, this, [this](const QByteArray &body, int status) {
        Q_D(RestReplyBase);
        // isFinished() is observably true while the subclass emits finished(...).
        d->finished = true;
        if (dispatchSuccess(body, status)) {
            d->success = true;
        } else {
            d->success = false; // dispatchSuccess() recorded the error
            Q_EMIT failed(d->error);
        }
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });

    connect(d->engine, &RestReply::failed, this, [this](const ClientError &error) {
        Q_D(RestReplyBase);
        d->finished = true;
        d->success = false;
        d->error = error;
        Q_EMIT failed(error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

RestReplyBase::~RestReplyBase() = default;

bool RestReplyBase::isFinished() const
{
    Q_D(const RestReplyBase);
    return d->finished;
}

bool RestReplyBase::isSuccess() const
{
    Q_D(const RestReplyBase);
    return d->success;
}

ClientError RestReplyBase::error() const
{
    Q_D(const RestReplyBase);
    return d->error;
}

RateLimit RestReplyBase::rateLimit() const
{
    Q_D(const RestReplyBase);
    return d->engine->rateLimit();
}

int RestReplyBase::retryCount() const
{
    Q_D(const RestReplyBase);
    return d->engine->retryCount();
}

void RestReplyBase::setAutoDelete(bool enabled)
{
    Q_D(RestReplyBase);
    d->autoDelete = enabled;
}

bool RestReplyBase::autoDelete() const
{
    Q_D(const RestReplyBase);
    return d->autoDelete;
}

void RestReplyBase::abort()
{
    Q_D(RestReplyBase);
    d->engine->abort();
}

void RestReplyBase::setError(const ClientError &error)
{
    Q_D(RestReplyBase);
    d->error = error;
}

bool RestReplyBase::parseJsonObject(const QByteArray &body, int httpStatus, QJsonObject &out)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        setError(ClientError(
                ClientError::Kind::Parse,
                QStringLiteral("invalid JSON response: %1").arg(parseError.errorString()),
                httpStatus));
        return false;
    }
    out = doc.object();
    return true;
}

QByteArray RestReplyBase::responseContentType() const
{
    Q_D(const RestReplyBase);
    return d->engine->contentType();
}

} // namespace Client
} // namespace QtOpenAi
