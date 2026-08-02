// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/GlobalClient.h>

#include <QtCore/QByteArray>
#include <QtCore/QScopedPointer>

#include <optional>

namespace QtOpenAi {
namespace Client {

// Where CachingInterceptor keeps response bodies.
//
// An interface rather than a class because the useful backends differ in kind:
// an in-memory one for a single process, a file-backed one that survives a
// restart, a shared one on a server. Only the four operations below distinguish
// them; expiry and eviction are the backend's business, since a store that
// cannot express "old" has no business pretending to.
//
// A key is opaque -- CachingInterceptor::cacheKey() produces it -- and a value
// is a response body verbatim.
class QTOPENAI_CLIENT_EXPORT ResponseCache
{
public:
    ResponseCache();
    virtual ~ResponseCache();

    // The stored body, or nothing on a miss. Non-const because a lookup is
    // allowed to change the store: expiring an entry it just found stale, or
    // moving it to the front of an LRU list.
    virtual std::optional<QByteArray> lookup(const QByteArray &key) = 0;
    virtual void insert(const QByteArray &key, const QByteArray &body) = 0;
    virtual void remove(const QByteArray &key) = 0;
    virtual void clear() = 0;
    // Entries currently held. Counts what is stored, not what is still fresh.
    virtual int count() const = 0;

private:
    Q_DISABLE_COPY(ResponseCache)
};

class MemoryResponseCachePrivate;

// The default backend: bodies in memory, oldest evicted first, entries expiring
// after a time limit.
//
// Both limits exist because both failure modes are real. Without a size limit a
// long-running process that varies its prompts grows without bound; without a
// time limit a cached answer outlives the question, and "the model said that an
// hour ago" is rarely the answer a user wants today. The defaults are
// deliberately modest -- 128 entries, five minutes -- because a cache that
// silently keeps more than expected is worse than one that misses.
class QTOPENAI_CLIENT_EXPORT MemoryResponseCache : public ResponseCache
{
public:
    explicit MemoryResponseCache(int maxEntries = 128);
    ~MemoryResponseCache() override;

    // Entries older than this are misses and are dropped when found. 0 disables
    // expiry. Default 300 (five minutes).
    void setTtlSeconds(int seconds);
    int ttlSeconds() const;

    // Hard ceiling on entries; the least recently inserted go first. Lowering it
    // evicts immediately. Default 128.
    void setMaxEntries(int entries);
    int maxEntries() const;

    std::optional<QByteArray> lookup(const QByteArray &key) override;
    void insert(const QByteArray &key, const QByteArray &body) override;
    void remove(const QByteArray &key) override;
    void clear() override;
    int count() const override;

private:
    QScopedPointer<MemoryResponseCachePrivate> d;
};

} // namespace Client
} // namespace QtOpenAi
