// SPDX-License-Identifier: MIT
#pragma once

// Internal (private) HTTP helpers shared by the reply implementations: parsing
// of rate-limit and Retry-After headers, and of an error response body. Not
// installed.

#include "QtOpenAi/Client/ClientError.h"
#include "QtOpenAi/Client/RetryPolicy.h"

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRandomGenerator>
#include <QtCore/QString>
#include <QtNetwork/QNetworkReply>

namespace QtOpenAi {
namespace Client {
namespace detail {

// Parse an OpenAI-style duration header value into milliseconds.
// Accepts plain seconds ("30", "1.5") and unit forms ("30ms", "1s", "6m0s").
// Returns -1 when the value cannot be parsed.
inline int durationToMs(const QByteArray &raw)
{
    const QByteArray value = raw.trimmed();
    if (value.isEmpty())
        return -1;

    // Plain number → seconds.
    bool ok = false;
    const double asSeconds = value.toDouble(&ok);
    if (ok)
        return static_cast<int>(asSeconds * 1000.0);

    // Unit form: concatenated <number><unit> segments (h, m, s, ms).
    double totalMs = 0.0;
    int i = 0;
    const int n = value.size();
    bool any = false;
    while (i < n) {
        int start = i;
        while (i < n && (isdigit(value[i]) || value[i] == '.' || value[i] == '-'))
            ++i;
        if (i == start)
            return -1;
        const double number = value.mid(start, i - start).toDouble(&ok);
        if (!ok)
            return -1;
        int unitStart = i;
        while (i < n && !isdigit(value[i]) && value[i] != '.' && value[i] != '-')
            ++i;
        const QByteArray unit = value.mid(unitStart, i - unitStart);
        if (unit == "ms")
            totalMs += number;
        else if (unit == "s")
            totalMs += number * 1000.0;
        else if (unit == "m")
            totalMs += number * 60000.0;
        else if (unit == "h")
            totalMs += number * 3600000.0;
        else
            return -1;
        any = true;
    }
    return any ? static_cast<int>(totalMs) : -1;
}

// Parse a Retry-After header (delta-seconds or an HTTP-date) into milliseconds.
inline int retryAfterToMs(const QByteArray &raw)
{
    const QByteArray value = raw.trimmed();
    if (value.isEmpty())
        return -1;
    bool ok = false;
    const int seconds = value.toInt(&ok);
    if (ok)
        return seconds * 1000;
    const QDateTime when = QDateTime::fromString(QString::fromLatin1(value), Qt::RFC2822Date);
    if (when.isValid()) {
        const qint64 ms = QDateTime::currentDateTimeUtc().msecsTo(when);
        return ms > 0 ? static_cast<int>(ms) : 0;
    }
    return -1;
}

// Collect rate-limit information from a reply's response headers.
inline RateLimit parseRateLimit(QNetworkReply *reply)
{
    RateLimit info;
    auto intHeader = [reply](const char *name, int &out) {
        const QByteArray raw = reply->rawHeader(name);
        if (!raw.isEmpty()) {
            bool ok = false;
            const int v = raw.trimmed().toInt(&ok);
            if (ok)
                out = v;
        }
    };
    intHeader("x-ratelimit-limit-requests", info.limitRequests);
    intHeader("x-ratelimit-remaining-requests", info.remainingRequests);
    intHeader("x-ratelimit-limit-tokens", info.limitTokens);
    intHeader("x-ratelimit-remaining-tokens", info.remainingTokens);

    if (reply->hasRawHeader("x-ratelimit-reset-requests"))
        info.resetRequestsMs = durationToMs(reply->rawHeader("x-ratelimit-reset-requests"));
    if (reply->hasRawHeader("x-ratelimit-reset-tokens"))
        info.resetTokensMs = durationToMs(reply->rawHeader("x-ratelimit-reset-tokens"));
    if (reply->hasRawHeader("Retry-After"))
        info.retryAfterMs = retryAfterToMs(reply->rawHeader("Retry-After"));
    return info;
}

// The error a failed exchange reports.
//
// OpenAI answers a failure with a JSON body carrying an `error` object, and
// when there is one its message, type and code are what a caller wants. When
// there is not -- a proxy's HTML, a truncated body, a transport failure that
// never reached the API at all -- the network layer's own description is all
// there is to go on, so it stands.
//
// `transportMessage` is QNetworkReply::errorString(); `status` the HTTP status,
// 0 when no response arrived. A status below 400 with an error here means the
// transport failed rather than the API, which is the Network/Http distinction.
inline ClientError errorFromBody(const QByteArray &body, const QString &transportMessage,
                                 int status)
{
    ClientError error(status >= 400 ? ClientError::Kind::Http : ClientError::Kind::Network,
                      transportMessage, status);

    const QJsonDocument document = QJsonDocument::fromJson(body);
    if (!document.isObject())
        return error;
    const QJsonObject errorObject = document.object().value(QStringLiteral("error")).toObject();
    if (errorObject.isEmpty())
        return error;

    // An `error` object is the API speaking for itself, so it replaces the
    // transport's account of what went wrong rather than adding to it -- and it
    // makes this an HTTP error whatever the status looked like.
    error = ClientError(ClientError::Kind::Http,
                        errorObject.value(QStringLiteral("message")).toString(transportMessage),
                        status);
    error.setType(errorObject.value(QStringLiteral("type")).toString());
    error.setCode(errorObject.value(QStringLiteral("code")).toString());
    return error;
}

// Compute the delay before the next retry, honouring Retry-After and jitter.
inline int retryDelayMs(const RetryPolicy &policy, int attempt, const RateLimit &rateLimit)
{
    int delay = policy.backoffDelayMs(attempt);
    if (policy.respectRetryAfter && rateLimit.retryAfterMs >= 0)
        delay = rateLimit.retryAfterMs;
    if (policy.jitter && delay > 0)
        delay = QRandomGenerator::global()->bounded(delay + 1);
    return delay;
}

} // namespace detail
} // namespace Client
} // namespace QtOpenAi
