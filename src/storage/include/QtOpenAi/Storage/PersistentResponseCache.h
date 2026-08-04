// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ResponseCache.h>
#include <QtOpenAi/Storage/GlobalStorage.h>

#include <QtCore/QScopedPointer>

namespace QtOpenAi {
namespace Storage {

class Store;
class PersistentResponseCachePrivate;

// A ResponseCache that keeps its bodies in a Store, so the cache survives a
// restart:
//
//     Storage::PersistentResponseCache store(&conversationStore);
//     Client::CachingInterceptor cache;
//     cache.setCache(&store);
//     client.addInterceptor(&cache);
//
// The store is not owned, and one store can hold the conversations, the cache
// and the metrics at once -- they are separate collections in it.
//
// Everything CachingInterceptor decides stays where it was: what may be cached
// at all, what the key hashes, that errors are not stored. This class adds only
// what a store cannot decide for itself, which is when an entry stops counting:
//
// * **A time limit**, because a cached answer outlives the question and one
//   from the last session is by definition from a while ago. Default 300s, the
//   same as MemoryResponseCache -- a *disk* cache tempts a longer default, but
//   the answer going stale is a property of the answer, not of where it is
//   kept.
// * **A count ceiling**, oldest first. Default 1024 rather than the in-memory
//   128: the cost of an entry here is disk, not resident memory, and a store
//   that is reopened every session is exactly where a bigger history pays off.
//
// Both are applied on insert, in one call into the store, so the cost is one
// statement over an index rather than a query per entry.
class QTOPENAI_STORAGE_EXPORT PersistentResponseCache : public Client::ResponseCache
{
public:
    // The store must be open before this cache is used; it is not opened here,
    // because the same store is usually already open for the conversations.
    explicit PersistentResponseCache(Store *store);
    ~PersistentResponseCache() override;

    Store *store() const;

    // Entries older than this are misses, and are dropped when found. 0
    // disables expiry. Default 300 (five minutes).
    void setTtlSeconds(int seconds);
    int ttlSeconds() const;

    // Hard ceiling on stored entries; the oldest go first. Default 1024. As in
    // MemoryResponseCache, a ceiling of 0 or less stores nothing at all rather
    // than meaning "unlimited" -- one spelling for "off" beats two.
    void setMaxEntries(int entries);
    int maxEntries() const;

    std::optional<QByteArray> lookup(const QByteArray &key) override;
    void insert(const QByteArray &key, const QByteArray &body) override;
    void remove(const QByteArray &key) override;
    void clear() override;
    int count() const override;

private:
    QScopedPointer<PersistentResponseCachePrivate> d;
};

} // namespace Storage
} // namespace QtOpenAi
