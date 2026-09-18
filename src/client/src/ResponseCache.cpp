// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ResponseCache.h"

#include <QtCore/QCache>
#include <QtCore/QDateTime>

#include <climits>

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

// QCache's cost is an int, so the budget is carried in whole KiB rather than
// bytes: a 64 MiB ceiling is 65536 of these, and a qint64 byte figure could not
// be expressed at all above 2 GiB. Anything non-empty costs at least one, so a
// flood of tiny bodies is bounded by the same number.
constexpr qint64 kCostUnit = 1024;

int costOf(qint64 bytes)
{
    return int(qBound(qint64(1), (bytes + kCostUnit - 1) / kCostUnit, qint64(INT_MAX)));
}

int budgetCost(qint64 bytes)
{
    return bytes <= 0 ? 0 : int(qBound(qint64(1), bytes / kCostUnit, qint64(INT_MAX)));
}

} // namespace

class MemoryResponseCachePrivate
{
public:
    // QCache does the eviction: an LRU with a cost budget, and the cost of an
    // entry is the size of the body it holds.
    QCache<QByteArray, Entry> entries;
    int ttlSeconds = 300;
    // Kept alongside the QCache so maxBytes() answers with what the caller set
    // rather than with what it rounded to in KiB.
    qint64 maxBytes = 64 * 1024 * 1024;
};

MemoryResponseCache::MemoryResponseCache()
    : d(new MemoryResponseCachePrivate)
{
    d->entries.setMaxCost(budgetCost(d->maxBytes));
}

MemoryResponseCache::~MemoryResponseCache() = default;

void MemoryResponseCache::setTtlSeconds(int seconds) { d->ttlSeconds = qMax(0, seconds); }
int MemoryResponseCache::ttlSeconds() const { return d->ttlSeconds; }

void MemoryResponseCache::setMaxBytes(qint64 bytes)
{
    d->maxBytes = qMax(qint64(0), bytes);
    d->entries.setMaxCost(budgetCost(d->maxBytes));
}

qint64 MemoryResponseCache::maxBytes() const { return d->maxBytes; }

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
    if (d->maxBytes <= 0)
        return;
    // QCache refuses -- and deletes -- an object costing more than the whole
    // budget, which is the wanted outcome: a body that cannot coexist with
    // anything else is not worth evicting everything else for. Stale entries
    // under the same key would otherwise survive the refusal, so drop it first.
    const int cost = costOf(body.size());
    if (cost > d->entries.maxCost()) {
        d->entries.remove(key);
        return;
    }
    auto *entry = new Entry {body, QDateTime::currentMSecsSinceEpoch()};
    d->entries.insert(key, entry, cost);
}

void MemoryResponseCache::remove(const QByteArray &key) { d->entries.remove(key); }
void MemoryResponseCache::clear() { d->entries.clear(); }
int MemoryResponseCache::count() const { return d->entries.size(); }

} // namespace Client
} // namespace QtOpenAi
