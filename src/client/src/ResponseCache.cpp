// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ResponseCache.h"

#include <QtCore/QCache>
#include <QtCore/QDateTime>

namespace QtOpenAi {
namespace Client {

ResponseCache::ResponseCache() = default;
ResponseCache::~ResponseCache() = default;

namespace {

struct Entry
{
    QByteArray body;
    qint64 storedAtMs = 0;
};

} // namespace

class MemoryResponseCachePrivate
{
public:
    // QCache does the eviction: it is an LRU with a cost budget, and one entry
    // costing one turns the budget into a count of entries.
    QCache<QByteArray, Entry> entries;
    int ttlSeconds = 300;
};

MemoryResponseCache::MemoryResponseCache(int maxEntries)
    : d(new MemoryResponseCachePrivate)
{
    d->entries.setMaxCost(qMax(0, maxEntries));
}

MemoryResponseCache::~MemoryResponseCache() = default;

void MemoryResponseCache::setTtlSeconds(int seconds) { d->ttlSeconds = qMax(0, seconds); }
int MemoryResponseCache::ttlSeconds() const { return d->ttlSeconds; }

void MemoryResponseCache::setMaxEntries(int entries) { d->entries.setMaxCost(qMax(0, entries)); }
int MemoryResponseCache::maxEntries() const { return d->entries.maxCost(); }

std::optional<QByteArray> MemoryResponseCache::lookup(const QByteArray &key)
{
    const Entry *entry = d->entries.object(key);
    if (!entry)
        return std::nullopt;

    if (d->ttlSeconds > 0) {
        const qint64 ageMs = QDateTime::currentMSecsSinceEpoch() - entry->storedAtMs;
        if (ageMs >= qint64(d->ttlSeconds) * 1000) {
            // Drop it here rather than leaving it to the eviction: an expired
            // entry that stays is a stale entry occupying a live slot.
            d->entries.remove(key);
            return std::nullopt;
        }
    }
    return entry->body;
}

void MemoryResponseCache::insert(const QByteArray &key, const QByteArray &body)
{
    if (d->entries.maxCost() <= 0)
        return;
    auto *entry = new Entry {body, QDateTime::currentMSecsSinceEpoch()};
    d->entries.insert(key, entry, 1);
}

void MemoryResponseCache::remove(const QByteArray &key) { d->entries.remove(key); }
void MemoryResponseCache::clear() { d->entries.clear(); }
int MemoryResponseCache::count() const { return d->entries.size(); }

} // namespace Client
} // namespace QtOpenAi
