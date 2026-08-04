// SPDX-License-Identifier: MIT
#include "QtOpenAi/Tools/HttpTools.h"

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace QtOpenAi {
namespace Tools {

HttpTools::HttpTools(QObject *parent)
    : QObject(parent)
{ }

HttpTools::~HttpTools() = default;

QStringList HttpTools::allowedHosts() const { return m_allowedHosts; }

void HttpTools::setAllowedHosts(const QStringList &hosts)
{
    m_allowedHosts.clear();
    for (const QString &host : hosts)
        addAllowedHost(host);
}

void HttpTools::addAllowedHost(const QString &host)
{
    // Normalised once here so the check is a plain comparison, and so a
    // difference in case cannot become a difference in policy.
    const QString normalised = host.trimmed().toLower();
    if (!normalised.isEmpty() && !m_allowedHosts.contains(normalised))
        m_allowedHosts.append(normalised);
}

bool HttpTools::requiresHttps() const { return m_requiresHttps; }
void HttpTools::setRequiresHttps(bool required) { m_requiresHttps = required; }

qint64 HttpTools::maxBytes() const { return m_maxBytes; }
void HttpTools::setMaxBytes(qint64 bytes) { m_maxBytes = qMax(qint64(0), bytes); }

int HttpTools::timeoutMs() const { return m_timeoutMs; }
void HttpTools::setTimeoutMs(int timeoutMs) { m_timeoutMs = qMax(0, timeoutMs); }

void HttpTools::setNetworkAccessManager(QNetworkAccessManager *manager) { m_manager = manager; }

QNetworkAccessManager *HttpTools::networkAccessManager() const
{
    if (!m_manager)
        const_cast<HttpTools *>(this)->m_manager
                = new QNetworkAccessManager(const_cast<HttpTools *>(this));
    return m_manager;
}

QString HttpTools::http_get(const QString &url)
{
    const auto refuseWith = [this, &url](const QString &reason) {
        Q_EMIT refused(url, reason);
        return reason;
    };

    const QUrl parsed(url);
    if (!parsed.isValid() || parsed.host().isEmpty())
        return refuseWith(QStringLiteral("that is not a valid absolute URL"));

    if (m_requiresHttps
        && parsed.scheme().compare(QLatin1String("https"), Qt::CaseInsensitive) != 0)
        return refuseWith(QStringLiteral("only https URLs may be fetched"));

    // The allow-list before anything else. With nothing allowed, nothing is
    // fetched -- which is what makes this deny-by-default rather than a filter
    // someone forgot to configure.
    if (!m_allowedHosts.contains(parsed.host().toLower())) {
        return refuseWith(QStringLiteral("%1 is not one of the hosts this tool may fetch from")
                                  .arg(parsed.host()));
    }

    QNetworkRequest request(parsed);
    // A redirect is a host the allow-list never approved; following one would
    // hand the decision to the server.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    if (m_timeoutMs > 0)
        request.setTransferTimeout(m_timeoutMs);

    QNetworkReply *reply = networkAccessManager()->get(request);

    QByteArray body;
    bool overLimit = false;

    // Enforced as the body arrives, not after: a cap applied to a download that
    // already happened is not a cap.
    connect(reply, &QNetworkReply::readyRead, reply, [this, reply, &body, &overLimit]() {
        body += reply->readAll();
        if (m_maxBytes > 0 && body.size() > m_maxBytes) {
            overLimit = true;
            reply->abort();
        }
    });

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    // A belt to the transfer timeout's braces: a server that trickles one byte
    // at a time never trips a *transfer* timeout, and would hold the turn open
    // for as long as it liked.
    QTimer guard;
    if (m_timeoutMs > 0) {
        guard.setSingleShot(true);
        connect(&guard, &QTimer::timeout, reply, &QNetworkReply::abort);
        guard.start(m_timeoutMs);
    }
    loop.exec();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QNetworkReply::NetworkError error = reply->error();
    reply->deleteLater();

    if (overLimit)
        return refuseWith(QStringLiteral("the response is larger than this tool may handle"));
    if (error != QNetworkReply::NoError && status == 0)
        return refuseWith(QStringLiteral("the request failed or timed out"));
    if (status >= 300 && status < 400)
        return refuseWith(QStringLiteral("the server redirected, which this tool does not follow"));
    if (status >= 400)
        return QStringLiteral("the server answered %1").arg(status);

    Q_EMIT fetched(url, status, body.size());
    return QString::fromUtf8(body);
}

} // namespace Tools
} // namespace QtOpenAi
