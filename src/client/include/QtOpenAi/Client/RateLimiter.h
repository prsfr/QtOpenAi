// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/GlobalClient.h>

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

#include <functional>
#include <memory>

namespace QtOpenAi {
namespace Client {

class ClientPrivate;
class RateLimiterPrivate;

// Throttles a Client so it does not have to be throttled by the provider.
//
//     RateLimiter limiter;
//     limiter.setMaxConcurrent(4);
//     limiter.setRequestsPerMinute(60);
//     client.setRateLimiter(&limiter);
//
// A 429 costs a round trip, a retry and sometimes a longer penalty than the
// wait would have been; staying under the limit is cheaper than recovering from
// crossing it. The three budgets are independent and any of them may be left at
// 0, which means "no limit":
//
//   * **maxConcurrent** -- requests in flight at once. The one worth setting
//     even when the provider imposes no limit, because it also bounds how much
//     of your own memory and how many sockets a burst can take.
//   * **requestsPerMinute** -- a rolling window, not a per-minute bucket. A
//     bucket lets a caller spend the whole budget in the last second of one
//     minute and the whole of the next in the first second of the next, which
//     is exactly the burst the limit exists to prevent.
//   * **tokensPerMinute** -- the same window over *estimated* prompt tokens.
//     The estimate is deliberately generous: a token budget that undercounts is
//     a budget that does not work.
//
// Requests over budget are queued and released in order, so a call still
// returns its reply immediately -- the reply simply has not started yet. This is
// invisible to calling code, which was always asynchronous.
//
// **Disabled by default**: a Client with no limiter installed queues nothing.
//
// Scope: this governs the one-shot endpoints. A streamed request is not gated,
// because a stream is held open for as long as the model is talking and
// counting one against a concurrency budget would mean a single long answer
// starving everything behind it. A cache hit is not gated either -- it makes no
// request, so there is no budget to spend.
class QTOPENAI_CLIENT_EXPORT RateLimiter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int maxConcurrent READ maxConcurrent WRITE setMaxConcurrent)
    Q_PROPERTY(int requestsPerMinute READ requestsPerMinute WRITE setRequestsPerMinute)
    Q_PROPERTY(int tokensPerMinute READ tokensPerMinute WRITE setTokensPerMinute)
public:
    explicit RateLimiter(QObject *parent = nullptr);
    ~RateLimiter() override;

    // 0 means no limit, for each of the three.
    void setMaxConcurrent(int count);
    int maxConcurrent() const;

    void setRequestsPerMinute(int count);
    int requestsPerMinute() const;

    void setTokensPerMinute(int count);
    int tokensPerMinute() const;

    // Waiting and running right now. Enough to report how far along a burst is.
    int queued() const;
    int inFlight() const;

    // Hold everything back for this long. What a 429 means for the whole client
    // rather than only for the request that received it: the provider is
    // telling you that you, not that request, are going too fast. The Client
    // calls this automatically when a response carries Retry-After.
    void pauseFor(int msecs);
    bool isPaused() const;

    // Drop the queue. Waiting requests are released immediately rather than
    // abandoned -- a limiter that is being torn down must not strand a caller
    // waiting for a reply that would never start.
    void flush();

Q_SIGNALS:
    void queueChanged(int queued, int inFlight);
    // Emitted when a provider's Retry-After put the whole client on hold.
    void pausedFor(int msecs);

private:
    // The gate the Client installs. A ticket is handed to `proceed` and holds
    // the slot for as long as it is alive, so a request that is destroyed while
    // waiting -- or while running -- gives its slot back without the limiter
    // having to watch for it.
    using Ticket = std::shared_ptr<void>;
    void schedule(int estimatedTokens, std::function<void(Ticket)> proceed);
    friend class ClientPrivate;

    Q_DECLARE_PRIVATE(RateLimiter)
    QScopedPointer<RateLimiterPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
