// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ResponseReply.h"

#include "HttpSupport_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkReply>

namespace QtOpenAi {
namespace Client {

class ResponseReplyPrivate
{
public:
    std::function<QNetworkReply *()> factory;
    RetryPolicy policy;
    QNetworkReply *networkReply = nullptr;
    Core::Response response;
    ClientError error;
    RateLimit rateLimit;
    int retryCount = 0;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

ResponseReply::ResponseReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                             QObject *parent)
    : QObject(parent)
    , d_ptr(new ResponseReplyPrivate)
{
    Q_D(ResponseReply);
    d->factory = std::move(requestFactory);
    d->policy = std::move(policy);

    // Kick off the first attempt on the next event-loop turn so callers can
    // connect to the signals before anything can fire.
    QTimer::singleShot(0, this, [this]() { start(); });
}

ResponseReply::~ResponseReply() = default;

void ResponseReply::start()
{
    Q_D(ResponseReply);
    d->networkReply = d->factory();
    d->networkReply->setParent(this);

    connect(d->networkReply, &QNetworkReply::finished, this, [this]() {
        Q_D(ResponseReply);
        QNetworkReply *reply = d->networkReply;
        const QByteArray body = reply->readAll();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        d->rateLimit = detail::parseRateLimit(reply);

        const bool networkError = reply->error() != QNetworkReply::NoError && status < 400;
        const bool httpError = status >= 400 || reply->error() != QNetworkReply::NoError;

        // Decide whether this failure is retryable and we still have budget.
        const bool retryable = (networkError && d->policy.retryOnNetworkError)
                               || (status >= 400 && d->policy.isRetryableStatus(status));
        if (retryable && d->retryCount < d->policy.maxRetries) {
            const int attempt = d->retryCount;
            ++d->retryCount;
            const int delay = detail::retryDelayMs(d->policy, attempt, d->rateLimit);
            reply->deleteLater();
            d->networkReply = nullptr;
            Q_EMIT retrying(d->retryCount, delay);
            QTimer::singleShot(delay, this, [this]() { start(); });
            return;
        }

        d->finished = true;
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);

        if (networkError) {
            d->success = false;
            d->error = ClientError(ClientError::Kind::Network, reply->errorString(), status);
        } else if (httpError) {
            QString message = reply->errorString();
            ClientError err(ClientError::Kind::Http, message, status);
            if (doc.isObject()) {
                const QJsonObject errorObject
                        = doc.object().value(QStringLiteral("error")).toObject();
                if (!errorObject.isEmpty()) {
                    message = errorObject.value(QStringLiteral("message")).toString(message);
                    err = ClientError(ClientError::Kind::Http, message, status);
                    err.setType(errorObject.value(QStringLiteral("type")).toString());
                    err.setCode(errorObject.value(QStringLiteral("code")).toString());
                }
            }
            d->success = false;
            d->error = err;
        } else if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            d->success = false;
            d->error = ClientError(
                    ClientError::Kind::Parse,
                    QStringLiteral("invalid JSON response: %1").arg(parseError.errorString()),
                    status);
        } else {
            d->response = Core::Response::fromJson(doc.object());
            d->success = true;
        }

        if (d->success)
            Q_EMIT finished(d->response);
        else
            Q_EMIT failed(d->error);
        Q_EMIT done();

        if (d->autoDelete)
            deleteLater();
    });
}

bool ResponseReply::isFinished() const
{
    Q_D(const ResponseReply);
    return d->finished;
}

bool ResponseReply::isSuccess() const
{
    Q_D(const ResponseReply);
    return d->success;
}

Core::Response ResponseReply::response() const
{
    Q_D(const ResponseReply);
    return d->response;
}

ClientError ResponseReply::error() const
{
    Q_D(const ResponseReply);
    return d->error;
}

RateLimit ResponseReply::rateLimit() const
{
    Q_D(const ResponseReply);
    return d->rateLimit;
}

int ResponseReply::retryCount() const
{
    Q_D(const ResponseReply);
    return d->retryCount;
}

void ResponseReply::setAutoDelete(bool enabled)
{
    Q_D(ResponseReply);
    d->autoDelete = enabled;
}

bool ResponseReply::autoDelete() const
{
    Q_D(const ResponseReply);
    return d->autoDelete;
}

void ResponseReply::abort()
{
    Q_D(ResponseReply);
    if (d->networkReply && d->networkReply->isRunning())
        d->networkReply->abort();
}

} // namespace Client
} // namespace QtOpenAi
