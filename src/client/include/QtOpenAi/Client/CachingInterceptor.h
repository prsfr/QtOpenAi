// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/Interceptor.h>

#include <QtCore/QScopedPointer>
#include <QtCore/QStringList>

namespace QtOpenAi {
namespace Client {

class ResponseCache;
class CachingInterceptorPrivate;

// Serves an identical request from a store instead of the network.
//
//     CachingInterceptor cache;
//     client.addInterceptor(&cache);
//
// Worth having for deterministic calls (`temperature: 0`), for a prompt an
// application re-issues as the user moves back and forth, and for tests -- each
// hit is a round trip and a bill that did not happen.
//
// **What is cached is an allow-list, and that is the whole safety story.** A
// POST is not idempotent in general: replaying `POST /files` from a cache would
// hand back the id of a file the caller believes it just created, and replaying
// `POST /fine_tuning/jobs` would hide a job that was never started. So only the
// endpoints that compute an answer from their input and change nothing --
// completions, embeddings, moderations -- are cacheable by default, and the
// list is the caller's to extend if their provider has others. GET and DELETE
// are never cached: a listing that cannot change is not a listing, and a
// cached DELETE is a lie.
//
// The key is a hash of the verb, the URL, the body **and the credential**. The
// credential is in there so a cache shared between two accounts cannot serve
// one account's answer to the other; it is hashed, never stored.
//
// Only 2xx responses are stored. An error is a state of the provider at a
// moment, not a property of the request, and caching one turns a blip into a
// sticky failure.
//
// Streaming is bypassed on its own: a stream never reaches afterResponse(),
// because it has no single body to store.
class QTOPENAI_CLIENT_EXPORT CachingInterceptor : public Interceptor
{
    Q_OBJECT
public:
    explicit CachingInterceptor(QObject *parent = nullptr);
    ~CachingInterceptor() override;

    // The store. Defaults to a MemoryResponseCache owned by this interceptor;
    // passing another one does not transfer ownership, and passing nullptr
    // restores the built-in one.
    ResponseCache *cache() const;
    void setCache(ResponseCache *cache);

    // Endpoint paths whose POSTs may be cached, matched as a suffix of the URL
    // path so a provider's prefix ("/openai/v1/...") does not matter. Setting
    // this replaces the list.
    QStringList cacheablePaths() const;
    void setCacheablePaths(const QStringList &paths);
    static QStringList defaultCacheablePaths();

    std::optional<InterceptedResponse> beforeRequest(InterceptedRequest &request) override;
    void afterResponse(const InterceptedResponse &response) override;

    // The key a request maps to. Exposed because a caller warming or
    // invalidating a store from outside needs the same answer this does.
    static QByteArray cacheKey(const InterceptedRequest &request);

Q_SIGNALS:
    // A request that was answered from the store, one that was not, and one
    // whose response was just added to it. Enough to measure a hit rate without
    // the cache having to keep counters nobody reads.
    void hit(const QUrl &url);
    void missed(const QUrl &url);
    void stored(const QUrl &url);

private:
    QScopedPointer<CachingInterceptorPrivate> d;
};

} // namespace Client
} // namespace QtOpenAi
