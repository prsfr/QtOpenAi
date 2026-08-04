// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>
#include <QtOpenAi/Core/Usage.h>

#include <QtCore/QHash>
#include <QtCore/QMetaType>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Client {

// What one request cost and how it went.
//
// Recorded by MetricsCollector as each reply finishes. `model` and `usage` are
// filled in only for the replies whose response reports them -- a file upload
// has no tokens and no model, and saying so with zeros beats inventing a
// number.
struct QTOPENAI_CLIENT_EXPORT RequestMetrics
{
    QString model;
    Core::Usage usage;

    // Wall-clock time from the request being created to the reply finishing,
    // retries and all -- what the caller actually waited.
    qint64 durationMs = 0;
    // Streamed replies only: the wait before the first fragment arrived, which
    // is what a user perceives as the latency. -1 when the reply was not
    // streamed, or produced nothing.
    qint64 timeToFirstTokenMs = -1;

    int httpStatus = 0; // 0 when no response arrived at all
    int retryCount = 0;
    bool ok = false;
    ClientError::Kind errorKind = ClientError::Kind::NoError;
    RateLimit rateLimit;
};

// Tokens and money for one model.
struct QTOPENAI_CLIENT_EXPORT ModelMetrics
{
    int requests = 0;
    qint64 promptTokens = 0;
    qint64 completionTokens = 0;
    qint64 totalTokens = 0;
    // US dollars, from the catalog's prices at the time each request was
    // recorded. Zero for a model the catalog has no price for -- an honest
    // "unknown", not "free".
    double cost = 0;

    ModelMetrics &operator+=(const ModelMetrics &other);
};

// Everything recorded so far.
struct QTOPENAI_CLIENT_EXPORT MetricsSnapshot
{
    int requests = 0;
    int successes = 0;
    int failures = 0;
    // Keyed by HTTP status; the 0 entry is the requests that never got a
    // response -- timeouts, DNS, TLS.
    QHash<int, int> failuresByStatus;

    qint64 totalDurationMs = 0;
    qint64 slowestDurationMs = 0;

    int streamedRequests = 0;
    qint64 totalTimeToFirstTokenMs = 0;

    QHash<QString, ModelMetrics> models;
    // The headroom the last response reported, which is the only one that still
    // says anything about now.
    RateLimit rateLimit;

    // Summed across models.
    ModelMetrics totals() const;
    double cost() const { return totals().cost; }

    // Zero when nothing has been recorded, rather than a division by it.
    double averageDurationMs() const;
    double averageTimeToFirstTokenMs() const;
};

} // namespace Client
} // namespace QtOpenAi

Q_DECLARE_METATYPE(QtOpenAi::Client::RequestMetrics)
Q_DECLARE_METATYPE(QtOpenAi::Client::ModelMetrics)
Q_DECLARE_METATYPE(QtOpenAi::Client::MetricsSnapshot)
