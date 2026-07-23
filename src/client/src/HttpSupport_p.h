// SPDX-License-Identifier: MIT
#pragma once

// Internal (private) HTTP helpers shared by the reply implementations:
// parsing of rate-limit and Retry-After headers. Not installed.

#include "QtOpenAi/Client/RetryPolicy.h"

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QRandomGenerator>
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
