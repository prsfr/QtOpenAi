// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ChatCompletionReply.h"

#include "HttpSupport_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRandomGenerator>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkReply>

namespace QtOpenAi {
namespace Client {

class ChatCompletionReplyPrivate
{
public:
    std::function<QNetworkReply *()> factory;
    RetryPolicy policy;
    QNetworkReply *networkReply = nullptr;
    Core::ChatCompletionResponse response;
    ClientError error;
    RateLimit rateLimit;
    int retryCount = 0;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

ChatCompletionReply::ChatCompletionReply(std::function<QNetworkReply *()> requestFactory,
                                         RetryPolicy policy, QObject *parent)
    : QObject(parent)
    , d_ptr(new ChatCompletionReplyPrivate)
{
    Q_D(ChatCompletionReply);
    d->factory = std::move(requestFactory);
    d->policy = std::move(policy);

    // Kick off the first attempt on the next event-loop turn so callers can
    // connect to the signals before anything can fire.
    QTimer::singleShot(0, this, [this]() { start(); });
}

ChatCompletionReply::~ChatCompletionReply() = default;

// Compute the delay before the next retry, honouring Retry-After and jitter.
static int retryDelayMs(const RetryPolicy &policy, int attempt, const RateLimit &rateLimit)
{
    int delay = policy.backoffDelayMs(attempt);
    if (policy.respectRetryAfter && rateLimit.retryAfterMs >= 0)
        delay = rateLimit.retryAfterMs;
    if (policy.jitter && delay > 0)
        delay = QRandomGenerator::global()->bounded(delay + 1);
    return delay;
}

void ChatCompletionReply::start()
{
    Q_D(ChatCompletionReply);
    d->networkReply = d->factory();
    d->networkReply->setParent(this);

    connect(d->networkReply, &QNetworkReply::finished, this, [this]() {
        Q_D(ChatCompletionReply);
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
            const int delay = retryDelayMs(d->policy, attempt, d->rateLimit);
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
            d->response = Core::ChatCompletionResponse::fromJson(doc.object());
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

bool ChatCompletionReply::isFinished() const
{
    Q_D(const ChatCompletionReply);
    return d->finished;
}

bool ChatCompletionReply::isSuccess() const
{
    Q_D(const ChatCompletionReply);
    return d->success;
}

Core::ChatCompletionResponse ChatCompletionReply::response() const
{
    Q_D(const ChatCompletionReply);
    return d->response;
}

ClientError ChatCompletionReply::error() const
{
    Q_D(const ChatCompletionReply);
    return d->error;
}

RateLimit ChatCompletionReply::rateLimit() const
{
    Q_D(const ChatCompletionReply);
    return d->rateLimit;
}

int ChatCompletionReply::retryCount() const
{
    Q_D(const ChatCompletionReply);
    return d->retryCount;
}

void ChatCompletionReply::setAutoDelete(bool enabled)
{
    Q_D(ChatCompletionReply);
    d->autoDelete = enabled;
}

bool ChatCompletionReply::autoDelete() const
{
    Q_D(const ChatCompletionReply);
    return d->autoDelete;
}

void ChatCompletionReply::abort()
{
    Q_D(ChatCompletionReply);
    if (d->networkReply && d->networkReply->isRunning())
        d->networkReply->abort();
}

} // namespace Client
} // namespace QtOpenAi
