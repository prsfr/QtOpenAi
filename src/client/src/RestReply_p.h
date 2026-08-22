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
    // Decides *when* the first attempt runs. Handed a callback it must
    // eventually invoke (or drop, if the request was abandoned while waiting).
    // This is where a rate limiter holds a request back.
    //
    // Only the first attempt is gated. A retry is the tail of a request that
    // already got through, and making it queue again would pin the slot it
    // occupies behind whatever is now ahead of it.
    using Gate = std::function<void(std::function<void()>)>;

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
    // One raw response header of the last successful response, matched
    // case-insensitively as HTTP requires. Empty when the header was absent.
    // Kept alongside the body because some endpoints put part of the result
    // there -- POST /realtime/calls returns the call id in `Location` and
    // nowhere else.
    QByteArray responseHeader(const QByteArray &name) const;
    void abort();

    // Must be installed before the event loop turns, i.e. in the same turn the
    // reply was constructed -- the first attempt is scheduled from there.
    void setGate(Gate gate);

Q_SIGNALS:
    // Emitted once per settled request whatever the outcome -- 2xx, HTTP error
    // or transport failure -- immediately before succeeded()/failed(), so an
    // observer sees the raw exchange without having to reassemble it from two
    // signals. `httpStatus` is 0 when no response arrived at all.
    void settled(const QByteArray &body, int httpStatus);
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
    QList<QPair<QByteArray, QByteArray>> m_responseHeaders;
    Gate m_gate;
    int m_retryCount = 0;
};

} // namespace Client
} // namespace QtOpenAi
