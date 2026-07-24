// SPDX-License-Identifier: MIT
#include "RestReply_p.h"

#include "HttpSupport_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkReply>

namespace QtOpenAi {
namespace Client {

RestReply::RestReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                     QObject *parent)
    : QObject(parent)
    , m_factory(std::move(requestFactory))
    , m_policy(std::move(policy))
{
    // Defer the first attempt so callers can connect before anything fires.
    QTimer::singleShot(0, this, [this]() { start(); });
}

RestReply::~RestReply() = default;

RateLimit RestReply::rateLimit() const { return m_rateLimit; }
int RestReply::retryCount() const { return m_retryCount; }
QByteArray RestReply::contentType() const { return m_contentType; }

void RestReply::abort()
{
    if (m_networkReply && m_networkReply->isRunning())
        m_networkReply->abort();
}

void RestReply::start()
{
    m_networkReply = m_factory();
    m_networkReply->setParent(this);

    connect(m_networkReply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = m_networkReply;
        const QByteArray body = reply->readAll();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        m_rateLimit = detail::parseRateLimit(reply);

        const bool networkError = reply->error() != QNetworkReply::NoError && status < 400;
        const bool httpError = status >= 400 || reply->error() != QNetworkReply::NoError;

        // Decide whether this failure is retryable and we still have budget.
        const bool retryable = (networkError && m_policy.retryOnNetworkError)
                               || (status >= 400 && m_policy.isRetryableStatus(status));
        if (retryable && m_retryCount < m_policy.maxRetries) {
            const int attempt = m_retryCount;
            ++m_retryCount;
            const int delay = detail::retryDelayMs(m_policy, attempt, m_rateLimit);
            reply->deleteLater();
            m_networkReply = nullptr;
            Q_EMIT retrying(m_retryCount, delay);
            QTimer::singleShot(delay, this, [this]() { start(); });
            return;
        }

        if (networkError) {
            Q_EMIT failed(ClientError(ClientError::Kind::Network, reply->errorString(), status));
            return;
        }
        if (httpError) {
            const QJsonDocument doc = QJsonDocument::fromJson(body);
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
            Q_EMIT failed(err);
            return;
        }

        m_contentType = reply->header(QNetworkRequest::ContentTypeHeader).toByteArray();
        Q_EMIT succeeded(body, status);
    });
}

} // namespace Client
} // namespace QtOpenAi
