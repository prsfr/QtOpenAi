// SPDX-License-Identifier: MIT
#include "RestReply_p.h"

#include "HttpSupport_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QPointer>
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
    // Defer the first attempt so callers can connect before anything fires --
    // and so a gate installed right after construction is in place by then.
    QTimer::singleShot(0, this, [this]() {
        if (!m_gate) {
            start();
            return;
        }
        // The gate may hold the callback for an arbitrary time, by which point
        // this reply may be gone; it must not resurrect a dead one.
        QPointer<RestReply> self(this);
        const Gate gate = std::move(m_gate);
        gate([self]() {
            if (self)
                self->start();
        });
    });
}

RestReply::~RestReply() = default;

RateLimit RestReply::rateLimit() const { return m_rateLimit; }
int RestReply::retryCount() const { return m_retryCount; }
QByteArray RestReply::contentType() const { return m_contentType; }

QByteArray RestReply::responseHeader(const QByteArray &name) const
{
    for (const auto &header : m_responseHeaders) {
        if (header.first.compare(name, Qt::CaseInsensitive) == 0)
            return header.second;
    }
    return {};
}

void RestReply::setGate(Gate gate) { m_gate = std::move(gate); }

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
        // An aborted reply is closed by the time it finishes, and reading a
        // closed device is a warning on the console rather than an error worth
        // reporting -- there is simply no body, which is what abort() means.
        const QByteArray body = reply->isOpen() ? reply->readAll() : QByteArray();
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
            Q_EMIT settled(body, status);
            Q_EMIT failed(ClientError(ClientError::Kind::Network, reply->errorString(), status));
            return;
        }
        if (httpError) {
            Q_EMIT settled(body, status);
            Q_EMIT failed(detail::errorFromBody(body, reply->errorString(), status));
            return;
        }

        m_contentType = reply->header(QNetworkRequest::ContentTypeHeader).toByteArray();
        m_responseHeaders = reply->rawHeaderPairs();
        Q_EMIT settled(body, status);
        Q_EMIT succeeded(body, status);
    });
}

} // namespace Client
} // namespace QtOpenAi
