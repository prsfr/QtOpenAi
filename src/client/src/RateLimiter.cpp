// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/RateLimiter.h"

#include <QtCore/QDateTime>
#include <QtCore/QQueue>
#include <QtCore/QTimer>

namespace QtOpenAi {
namespace Client {

namespace {
constexpr qint64 kWindowMs = 60000;
}

class RateLimiterPrivate
{
public:
    explicit RateLimiterPrivate(RateLimiter *limiter)
        : q(limiter)
    { }

    struct Pending
    {
        int tokens = 0;
        std::function<void(RateLimiter::Ticket)> proceed;
    };

    struct Spent
    {
        qint64 atMs = 0;
        int tokens = 0;
    };

    RateLimiter *q;
    int maxConcurrent = 0;
    int requestsPerMinute = 0;
    int tokensPerMinute = 0;
    int inFlight = 0;
    qint64 pausedUntilMs = 0;
    QQueue<Pending> queue;
    // What the rolling window has already spent, newest last.
    QList<Spent> window;

    static qint64 now() { return QDateTime::currentMSecsSinceEpoch(); }

    // Forget what has aged out. Called before every budget decision, so the
    // window is a window rather than a growing list.
    void prune(qint64 atMs)
    {
        int keep = 0;
        while (keep < window.size() && atMs - window.at(keep).atMs >= kWindowMs)
            ++keep;
        if (keep > 0)
            window.remove(0, keep);
    }

    int spentRequests() const { return window.size(); }

    int spentTokens() const
    {
        int total = 0;
        for (const Spent &spent : window)
            total += spent.tokens;
        return total;
    }

    // When the budget will next free up, or -1 if only a finishing request can
    // free it. Waking exactly then beats polling, and beats waiting a fixed
    // second that may be far too long or far too short.
    qint64 nextExpiryMs(qint64 atMs) const
    {
        if (window.isEmpty())
            return -1;
        return kWindowMs - (atMs - window.first().atMs);
    }

    bool canDispatch(const Pending &pending, qint64 atMs) const
    {
        if (pausedUntilMs > atMs)
            return false;
        if (maxConcurrent > 0 && inFlight >= maxConcurrent)
            return false;
        if (requestsPerMinute > 0 && spentRequests() >= requestsPerMinute)
            return false;
        // A single request larger than the whole budget would never fit; let it
        // through on an empty window rather than wedge the queue forever.
        if (tokensPerMinute > 0 && !window.isEmpty()
            && spentTokens() + pending.tokens > tokensPerMinute)
            return false;
        return true;
    }

    void pump()
    {
        const qint64 atMs = now();
        prune(atMs);

        while (!queue.isEmpty() && canDispatch(queue.head(), atMs)) {
            const Pending pending = queue.dequeue();
            ++inFlight;
            window.append({atMs, pending.tokens});
            // The ticket is the slot. Whoever holds it holds the slot, and
            // dropping it -- by finishing, or by being destroyed while waiting
            // -- gives it back. Nothing has to remember to call release().
            RateLimiter::Ticket ticket(nullptr, [this](void *) { finish(); });
            pending.proceed(std::move(ticket));
        }

        if (!queue.isEmpty())
            wakeWhenBudgetFrees(atMs);
        Q_EMIT q->queueChanged(queue.size(), inFlight);
    }

    void wakeWhenBudgetFrees(qint64 atMs)
    {
        qint64 delay = -1;
        if (pausedUntilMs > atMs)
            delay = pausedUntilMs - atMs;
        if (requestsPerMinute > 0 || tokensPerMinute > 0) {
            const qint64 expiry = nextExpiryMs(atMs);
            if (expiry >= 0 && (delay < 0 || expiry < delay))
                delay = expiry;
        }
        // A concurrency-only limit frees up when a request finishes, not on a
        // clock, so there is nothing to wake for.
        if (delay < 0)
            return;
        QTimer::singleShot(int(qMax<qint64>(delay, 1)), q, [this]() { pump(); });
    }

    void finish()
    {
        if (inFlight > 0)
            --inFlight;
        pump();
    }
};

RateLimiter::RateLimiter(QObject *parent)
    : QObject(parent)
    , d_ptr(new RateLimiterPrivate(this))
{ }

RateLimiter::~RateLimiter()
{
    // Anything still waiting has to be let go. A caller holding a reply that
    // would now never start would wait for it forever, and a limiter being torn
    // down is no longer limiting anything.
    flush();
}

void RateLimiter::setMaxConcurrent(int count)
{
    Q_D(RateLimiter);
    d->maxConcurrent = qMax(0, count);
    d->pump();
}

int RateLimiter::maxConcurrent() const
{
    Q_D(const RateLimiter);
    return d->maxConcurrent;
}

void RateLimiter::setRequestsPerMinute(int count)
{
    Q_D(RateLimiter);
    d->requestsPerMinute = qMax(0, count);
    d->pump();
}

int RateLimiter::requestsPerMinute() const
{
    Q_D(const RateLimiter);
    return d->requestsPerMinute;
}

void RateLimiter::setTokensPerMinute(int count)
{
    Q_D(RateLimiter);
    d->tokensPerMinute = qMax(0, count);
    d->pump();
}

int RateLimiter::tokensPerMinute() const
{
    Q_D(const RateLimiter);
    return d->tokensPerMinute;
}

int RateLimiter::queued() const
{
    Q_D(const RateLimiter);
    return d->queue.size();
}

int RateLimiter::inFlight() const
{
    Q_D(const RateLimiter);
    return d->inFlight;
}

void RateLimiter::pauseFor(int msecs)
{
    Q_D(RateLimiter);
    if (msecs <= 0)
        return;
    const qint64 until = RateLimiterPrivate::now() + msecs;
    // Never shorten a pause already in force: two 429s in a row mean the second
    // provider's answer is the more recent, not the more lenient.
    if (until <= d->pausedUntilMs)
        return;
    d->pausedUntilMs = until;
    Q_EMIT pausedFor(msecs);
    d->pump();
}

bool RateLimiter::isPaused() const
{
    Q_D(const RateLimiter);
    return d->pausedUntilMs > RateLimiterPrivate::now();
}

void RateLimiter::flush()
{
    Q_D(RateLimiter);
    // Released, not dropped: a caller waiting on a reply that will now never
    // start would wait forever, which is a worse outcome than going over
    // budget once while the limiter is being taken away.
    QQueue<RateLimiterPrivate::Pending> waiting;
    waiting.swap(d->queue);
    while (!waiting.isEmpty())
        waiting.dequeue().proceed(Ticket {});
    Q_EMIT queueChanged(0, d->inFlight);
}

void RateLimiter::schedule(int estimatedTokens, std::function<void(Ticket)> proceed)
{
    Q_D(RateLimiter);
    d->queue.enqueue({qMax(0, estimatedTokens), std::move(proceed)});
    d->pump();
}

} // namespace Client
} // namespace QtOpenAi
