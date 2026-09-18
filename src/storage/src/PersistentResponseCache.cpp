// SPDX-License-Identifier: MIT
#include "QtOpenAi/Storage/PersistentResponseCache.h"

#include "QtOpenAi/Storage/Store.h"

#include <QtCore/QElapsedTimer>

namespace QtOpenAi {
namespace Storage {

// How often the age sweep may run, at most. The ceiling is enforced on every
// insert because it is a hard bound on what the store holds; the age bound is
// not, because lookup() already refuses and drops a stale entry the moment it
// finds one. So the sweep is housekeeping, and a minute of slack in housekeeping
// costs nothing an application can observe.
constexpr qint64 kSweepIntervalMs = 60 * 1000;

class PersistentResponseCachePrivate
{
public:
    Store *store = nullptr;
    int ttlSeconds = 300;
    int maxEntries = 1024;

    // Never restarted, so elapsed() is monotonic from construction; the first
    // insert sweeps because 0 is already past the interval from -kSweepIntervalMs.
    QElapsedTimer since;
    qint64 lastSweepMs = -kSweepIntervalMs;

    // The instant before which an entry is stale, or an invalid QDateTime when
    // expiry is off -- which is also what the store reads as "no age bound".
    QDateTime cutoff() const
    {
        return ttlSeconds > 0 ? QDateTime::currentDateTimeUtc().addSecs(-ttlSeconds) : QDateTime();
    }
};

PersistentResponseCache::PersistentResponseCache(Store *store)
    : d(new PersistentResponseCachePrivate)
{
    d->store = store;
    d->since.start();
}

PersistentResponseCache::~PersistentResponseCache() = default;

Store *PersistentResponseCache::store() const { return d->store; }

void PersistentResponseCache::setTtlSeconds(int seconds) { d->ttlSeconds = qMax(0, seconds); }
int PersistentResponseCache::ttlSeconds() const { return d->ttlSeconds; }

void PersistentResponseCache::setMaxEntries(int entries) { d->maxEntries = entries; }
int PersistentResponseCache::maxEntries() const { return d->maxEntries; }

std::optional<QByteArray> PersistentResponseCache::lookup(const QByteArray &key)
{
    if (!d->store)
        return std::nullopt;
    const std::optional<CachedResponse> entry = d->store->cachedResponse(key);
    if (!entry)
        return std::nullopt;

    const QDateTime cutoff = d->cutoff();
    if (cutoff.isValid() && entry->storedAt < cutoff) {
        // Dropped here rather than left to the next prune: a stale entry that
        // stays is a stale entry occupying a live slot.
        d->store->removeCachedResponse(key);
        return std::nullopt;
    }
    return entry->body;
}

void PersistentResponseCache::insert(const QByteArray &key, const QByteArray &body)
{
    if (!d->store || d->maxEntries <= 0)
        return;
    CachedResponse entry;
    entry.key = key;
    entry.body = body;
    entry.storedAt = QDateTime::currentDateTimeUtc();

    // The insert and the prune it triggers are one batch: every cached
    // response pays for this pair, and on a backend with transactions the
    // difference is one commit here rather than one per statement.
    Store::Batch batch(d->store);
    if (!d->store->saveCachedResponse(entry)) {
        batch.abort();
        return;
    }

    // Pruning used to run in full on every insert, which meant every cached
    // response paid for a pass over the whole cache -- on the JSON backend, a
    // read and a parse of every file in it, to discover in the common case that
    // there was nothing to drop.
    //
    // The two bounds are not equally urgent. The ceiling is a promise about how
    // much the store holds, so it is passed every time -- and both backends
    // answer "already under it" from something they needed anyway, so asking
    // costs nothing. The age bound is not a promise about the store's contents at
    // all: a stale entry is never served, because lookup() drops it when it finds
    // it. Sweeping for it is housekeeping, and it runs at most once a minute.
    const qint64 now = d->since.elapsed();
    const bool sweepAge = d->ttlSeconds > 0 && now - d->lastSweepMs >= kSweepIntervalMs;
    if (sweepAge)
        d->lastSweepMs = now;

    d->store->pruneCachedResponses(d->maxEntries, sweepAge ? d->cutoff() : QDateTime());
}

void PersistentResponseCache::remove(const QByteArray &key)
{
    if (d->store)
        d->store->removeCachedResponse(key);
}

void PersistentResponseCache::clear()
{
    if (d->store)
        d->store->clearCachedResponses();
}

int PersistentResponseCache::count() const
{
    // Counts what is stored, not what is still fresh -- the same as every other
    // ResponseCache, whose count() says how much is held rather than how much
    // would still be served.
    return d->store ? d->store->cachedResponseCount() : 0;
}

} // namespace Storage
} // namespace QtOpenAi
