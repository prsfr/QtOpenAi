// SPDX-License-Identifier: MIT
#pragma once

// Internal transport engine shared by the typed Client replies. Runs one
// request with the configured retry policy, parses rate-limit headers, and
// turns transport/HTTP failures into a ClientError. It performs no JSON payload
// parsing — on a 2xx it hands the raw body to the caller, which decodes it into
// the appropriate value type. Not installed / not part of the public API.

#include "QtOpenAi/Client/ClientError.h"
#include "QtOpenAi/Client/RetryPolicy.h"

#include <QtCore/QByteArray>
#include <QtCore/QObject>

#include <functional>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class RestReply : public QObject
{
    Q_OBJECT
public:
    // Constructed with a factory that (re)issues the underlying network request,
    // so the engine can transparently retry per the supplied policy. The first
    // attempt is scheduled on the next event-loop turn.
    RestReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
              QObject *parent = nullptr);
    ~RestReply() override;

    RateLimit rateLimit() const;
    int retryCount() const;
    // Content-Type of the last successful response (e.g. "audio/mpeg"); useful
    // for endpoints that return a binary blob rather than JSON.
    QByteArray contentType() const;
    void abort();

Q_SIGNALS:
    // Emitted once on a 2xx response with the raw body and status code.
    void succeeded(const QByteArray &body, int httpStatus);
    // Emitted once on a terminal transport/HTTP failure.
    void failed(const QtOpenAi::Client::ClientError &error);
    // Emitted before each scheduled retry (1-based attempt, delay in ms).
    void retrying(int attempt, int delayMs);

private:
    void start();

    std::function<QNetworkReply *()> m_factory;
    RetryPolicy m_policy;
    QNetworkReply *m_networkReply = nullptr;
    RateLimit m_rateLimit;
    QByteArray m_contentType;
    int m_retryCount = 0;
};

} // namespace Client
} // namespace QtOpenAi
