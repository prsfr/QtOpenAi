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

} // namespace Client
} // namespace QtOpenAi
