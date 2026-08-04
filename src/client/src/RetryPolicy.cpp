// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/RetryPolicy.h"

#include <algorithm>
#include <cmath>

namespace QtOpenAi {
namespace Client {

int RetryPolicy::backoffDelayMs(int attempt) const
{
    if (initialDelayMs <= 0)
        return 0;
    const double raw = initialDelayMs * std::pow(multiplier, std::max(0, attempt));
    const double capped = std::min<double>(raw, maxDelayMs > 0 ? maxDelayMs : raw);
    return static_cast<int>(capped);
}

namespace {

// -1 is "the provider did not send this header", and that is not the same as
// zero remaining requests, so it is written as an absent key rather than as a
// number a reader would have to know to treat specially.
void putIfPresent(QJsonObject &json, QLatin1String key, int value)
{
    if (value >= 0)
        json.insert(key, value);
}

int valueOrAbsent(const QJsonObject &json, QLatin1String key)
{
    const QJsonValue value = json.value(key);
    return value.isUndefined() ? -1 : value.toInt(-1);
}

} // namespace

QJsonObject RateLimit::toJson() const
{
    QJsonObject json;
    putIfPresent(json, QLatin1String("limit_requests"), limitRequests);
    putIfPresent(json, QLatin1String("remaining_requests"), remainingRequests);
    putIfPresent(json, QLatin1String("limit_tokens"), limitTokens);
    putIfPresent(json, QLatin1String("remaining_tokens"), remainingTokens);
    putIfPresent(json, QLatin1String("reset_requests_ms"), resetRequestsMs);
    putIfPresent(json, QLatin1String("reset_tokens_ms"), resetTokensMs);
    putIfPresent(json, QLatin1String("retry_after_ms"), retryAfterMs);
    return json;
}

RateLimit RateLimit::fromJson(const QJsonObject &json)
{
    RateLimit limit;
    limit.limitRequests = valueOrAbsent(json, QLatin1String("limit_requests"));
    limit.remainingRequests = valueOrAbsent(json, QLatin1String("remaining_requests"));
    limit.limitTokens = valueOrAbsent(json, QLatin1String("limit_tokens"));
    limit.remainingTokens = valueOrAbsent(json, QLatin1String("remaining_tokens"));
    limit.resetRequestsMs = valueOrAbsent(json, QLatin1String("reset_requests_ms"));
    limit.resetTokensMs = valueOrAbsent(json, QLatin1String("reset_tokens_ms"));
    limit.retryAfterMs = valueOrAbsent(json, QLatin1String("retry_after_ms"));
    return limit;
}

} // namespace Client
} // namespace QtOpenAi
