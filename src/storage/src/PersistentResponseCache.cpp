// SPDX-License-Identifier: MIT
#include "QtOpenAi/Storage/PersistentResponseCache.h"

#include "QtOpenAi/Storage/Store.h"

namespace QtOpenAi {
namespace Storage {

class PersistentResponseCachePrivate
{
public:
    Store *store = nullptr;
    int ttlSeconds = 300;
    int maxEntries = 1024;

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
    if (d->store->saveCachedResponse(entry))
        d->store->pruneCachedResponses(d->maxEntries, d->cutoff());
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
