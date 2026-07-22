// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/GlobalClient.h>

#include <QtCore/QList>
#include <QtCore/QMetaType>

namespace QtOpenAi {
namespace Client {

// Configuration for automatic retries with exponential backoff.
//
// A request is retried when it fails with a network error (if enabled) or with
// one of the retryable HTTP statuses (429 and 5xx by default). The delay grows
// geometrically (initialDelay * multiplier^attempt), is capped at maxDelay, and
// — unless disabled — is overridden by a server `Retry-After` header when the
// response carries one. Optional full jitter spreads retries to avoid stampedes.
struct QTOPENAI_CLIENT_EXPORT RetryPolicy
{
    int maxRetries = 2; // additional attempts after the first try
    int initialDelayMs = 500;
    int maxDelayMs = 20000;
    double multiplier = 2.0;
    bool jitter = true; // apply full jitter to the computed delay
    bool retryOnNetworkError = true;
    bool respectRetryAfter = true; // honour a Retry-After response header
    QList<int> retryableStatuses = {429, 500, 502, 503, 504};

    bool isRetryableStatus(int status) const { return retryableStatuses.contains(status); }

    // Base backoff (ignoring Retry-After / jitter) for a 0-based attempt index.
    int backoffDelayMs(int attempt) const;

    // Disabled policy: never retries.
    static RetryPolicy none() { return RetryPolicy {0, 0, 0, 1.0, false, false, false, {}}; }
};

// Rate-limit information parsed from response headers (`x-ratelimit-*` and
// `Retry-After`). Fields are -1 when the corresponding header is absent.
struct QTOPENAI_CLIENT_EXPORT RateLimit
{
    int limitRequests = -1;
    int remainingRequests = -1;
    int limitTokens = -1;
    int remainingTokens = -1;
    // Reset durations in milliseconds (from `x-ratelimit-reset-*`), -1 if absent.
    int resetRequestsMs = -1;
    int resetTokensMs = -1;
    // `Retry-After` in milliseconds, -1 if absent.
    int retryAfterMs = -1;

    bool isValid() const
    {
        return limitRequests >= 0 || remainingRequests >= 0 || limitTokens >= 0
               || remainingTokens >= 0 || retryAfterMs >= 0;
    }
};

} // namespace Client
} // namespace QtOpenAi

Q_DECLARE_METATYPE(QtOpenAi::Client::RateLimit)
