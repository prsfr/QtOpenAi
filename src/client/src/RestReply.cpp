// SPDX-License-Identifier: MIT
#include "RestReply_p.h"

#include "HttpSupport_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
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

RestReply::~RestReply()
{
    // The reply belongs to the manager, so it is not freed with this engine by
    // parentage; free it here, without letting it call back into a half-gone
    // engine. Deleting an unfinished reply aborts it, as it always did.
    if (m_networkReply) {
        m_networkReply->disconnect(this);
        delete m_networkReply;
    }
}

void RestReply::failClientGone()
{
    if (m_settled)
        return;
    m_settled = true;
    Q_EMIT settled(QByteArray(), 0);
    Q_EMIT failed(
            ClientError(ClientError::Kind::Network, QStringLiteral("client no longer available")));
}

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
    // A retry timer or a rate-limiter gate can fire after the outcome is known
    // -- the manager died meanwhile -- and must not issue anything then.
    if (m_settled || m_managerGone)
        return;
    m_networkReply = m_factory();
    if (!m_networkReply) {
        // Queued like the manager-destroyed path: a rate limiter releases its
        // waiting requests from its destructor, which can be the Client's.
        m_managerGone = true;
        QTimer::singleShot(0, this, &RestReply::failClientGone);
        return;
    }
    if (!m_networkReply->parent())
        m_networkReply->setParent(this);

    // The manager deletes its replies when it is destroyed, so the reply is
    // never left behind holding pointers into a freed manager (#203). That
    // ends the request without a finished(), so the failure is delivered from
    // here, queued: the manager's destruction is often the Client's, and an
    // application slot must not run inside it.
    QNetworkAccessManager *manager = m_networkReply->manager();
    if (manager && !m_watchingManager) {
        m_watchingManager = true;
        connect(manager, &QObject::destroyed, this, [this]() {
            m_managerGone = true;
            QTimer::singleShot(0, this, &RestReply::failClientGone);
        });
    }

    connect(m_networkReply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = m_networkReply;
        // An aborted reply is closed by the time it finishes, and reading a
        // closed device is a warning on the console rather than an error worth
        // reporting -- there is simply no body, which is what abort() means.
        const QByteArray body = reply->isOpen() ? reply->readAll() : QByteArray();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        m_rateLimit = detail::parseRateLimit(reply);

        // A refused cross-origin redirect is an answer rather than a transport
        // hiccup -- sending it again would be refused again -- so it is neither
        // retried nor reported as a network error.
        const bool networkError = reply->error() != QNetworkReply::NoError && status < 400
                                  && reply->error() != QNetworkReply::InsecureRedirectError;
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

        // Everything is read off the reply before the first emission: a slot
        // on settled() may delete the Client, and with it the manager that
        // owns -- and frees -- the reply.
        m_settled = true;
        const QString transportMessage = reply->errorString();
        const QNetworkReply::NetworkError transportError = reply->error();
        if (networkError) {
            Q_EMIT settled(body, status);
            Q_EMIT failed(ClientError(ClientError::Kind::Network, transportMessage, status));
            return;
        }
        if (httpError) {
            Q_EMIT settled(body, status);
            Q_EMIT failed(detail::errorFromBody(body, transportMessage, status, transportError));
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
