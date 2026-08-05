// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/Metrics.h"

namespace QtOpenAi {
namespace Client {

ModelMetrics &ModelMetrics::operator+=(const ModelMetrics &other)
{
    requests += other.requests;
    promptTokens += other.promptTokens;
    completionTokens += other.completionTokens;
    totalTokens += other.totalTokens;
    cost += other.cost;
    return *this;
}

ModelMetrics MetricsSnapshot::totals() const
{
    ModelMetrics total;
    for (const ModelMetrics &model : models)
        total += model;
    return total;
}

double MetricsSnapshot::averageDurationMs() const
{
    return requests > 0 ? double(totalDurationMs) / requests : 0.0;
}

double MetricsSnapshot::averageTimeToFirstTokenMs() const
{
    return streamedRequests > 0 ? double(totalTimeToFirstTokenMs) / streamedRequests : 0.0;
}

QJsonObject ModelMetrics::toJson() const
{
    return QJsonObject {{QLatin1String("requests"), requests},
                        {QLatin1String("prompt_tokens"), promptTokens},
                        {QLatin1String("completion_tokens"), completionTokens},
                        {QLatin1String("total_tokens"), totalTokens},
                        {QLatin1String("cost"), cost}};
}

ModelMetrics ModelMetrics::fromJson(const QJsonObject &json)
{
    ModelMetrics metrics;
    metrics.requests = json.value(QLatin1String("requests")).toInt();
    metrics.promptTokens = json.value(QLatin1String("prompt_tokens")).toInteger();
    metrics.completionTokens = json.value(QLatin1String("completion_tokens")).toInteger();
    metrics.totalTokens = json.value(QLatin1String("total_tokens")).toInteger();
    metrics.cost = json.value(QLatin1String("cost")).toDouble();
    return metrics;
}

QJsonObject MetricsSnapshot::toJson() const
{
    QJsonObject byStatus;
    for (auto it = failuresByStatus.constBegin(); it != failuresByStatus.constEnd(); ++it) {
        // JSON object keys are strings; the status is the number it always was
        // and comes back through toInt() on load.
        byStatus.insert(QString::number(it.key()), it.value());
    }

    QJsonObject byModel;
    for (auto it = models.constBegin(); it != models.constEnd(); ++it)
        byModel.insert(it.key(), it.value().toJson());

    QJsonObject json {{QLatin1String("requests"), requests},
                      {QLatin1String("successes"), successes},
                      {QLatin1String("failures"), failures},
                      {QLatin1String("failures_by_status"), byStatus},
                      {QLatin1String("total_duration_ms"), totalDurationMs},
                      {QLatin1String("slowest_duration_ms"), slowestDurationMs},
                      {QLatin1String("streamed_requests"), streamedRequests},
                      {QLatin1String("total_time_to_first_token_ms"), totalTimeToFirstTokenMs},
                      {QLatin1String("models"), byModel}};
    // Only when the provider said something: an all -1 rate limit is the
    // absence of the headers, and writing it back would claim they arrived.
    if (rateLimit.isValid())
        json.insert(QLatin1String("rate_limit"), rateLimit.toJson());
    return json;
}

MetricsSnapshot MetricsSnapshot::fromJson(const QJsonObject &json)
{
    MetricsSnapshot snapshot;
    snapshot.requests = json.value(QLatin1String("requests")).toInt();
    snapshot.successes = json.value(QLatin1String("successes")).toInt();
    snapshot.failures = json.value(QLatin1String("failures")).toInt();
    snapshot.totalDurationMs = json.value(QLatin1String("total_duration_ms")).toInteger();
    snapshot.slowestDurationMs = json.value(QLatin1String("slowest_duration_ms")).toInteger();
    snapshot.streamedRequests = json.value(QLatin1String("streamed_requests")).toInt();
    snapshot.totalTimeToFirstTokenMs
            = json.value(QLatin1String("total_time_to_first_token_ms")).toInteger();

    const QJsonObject byStatus = json.value(QLatin1String("failures_by_status")).toObject();
    for (auto it = byStatus.constBegin(); it != byStatus.constEnd(); ++it)
        snapshot.failuresByStatus.insert(it.key().toInt(), it.value().toInt());

    const QJsonObject byModel = json.value(QLatin1String("models")).toObject();
    for (auto it = byModel.constBegin(); it != byModel.constEnd(); ++it)
        snapshot.models.insert(it.key(), ModelMetrics::fromJson(it.value().toObject()));

    snapshot.rateLimit = RateLimit::fromJson(json.value(QLatin1String("rate_limit")).toObject());
    return snapshot;
}

} // namespace Client
} // namespace QtOpenAi
