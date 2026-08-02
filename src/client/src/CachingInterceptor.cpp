// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/CachingInterceptor.h"

#include "QtOpenAi/Client/ResponseCache.h"

#include <QtCore/QCryptographicHash>

namespace QtOpenAi {
namespace Client {

namespace {

// Header names whose value goes into the key. Two accounts hitting one shared
// cache must not see each other's answers, and the account is only visible in
// the credential -- so the credential is part of the identity of the request.
// It is hashed with everything else and never stored.
const char *const kIdentityHeaders[] = {"Authorization", "api-key", "OpenAI-Organization",
                                        "OpenAI-Project"};

} // namespace

class CachingInterceptorPrivate
{
public:
    MemoryResponseCache builtIn;
    ResponseCache *cache = nullptr; // null means builtIn
    QStringList cacheablePaths = CachingInterceptor::defaultCacheablePaths();

    // Asked on the way out and again on the way back. It has to be one function:
    // a lookup rule and a store rule that drift apart is a cache that answers
    // questions it was never allowed to remember.
    bool isCacheable(const InterceptedRequest &request) const
    {
        if (request.method != "POST")
            return false;
        // Suffix rather than equality: the same endpoint is "/v1/embeddings" at
        // OpenAI and "/openai/v1/embeddings" at Azure.
        const QString path = request.url().path();
        for (const QString &candidate : cacheablePaths) {
            if (path.endsWith(candidate))
                return true;
        }
        return false;
    }
};

CachingInterceptor::CachingInterceptor(QObject *parent)
    : Interceptor(parent)
    , d(new CachingInterceptorPrivate)
{ }

CachingInterceptor::~CachingInterceptor() = default;

QStringList CachingInterceptor::defaultCacheablePaths()
{
    // Endpoints that compute an answer from their input and change nothing.
    // Everything else -- creating a file, starting a job, storing a response --
    // would be a different call the second time, so replaying it from a store
    // is not a saving but a wrong answer.
    return {QStringLiteral("/chat/completions"), QStringLiteral("/completions"),
            QStringLiteral("/embeddings"), QStringLiteral("/moderations")};
}

ResponseCache *CachingInterceptor::cache() const { return d->cache ? d->cache : &d->builtIn; }

void CachingInterceptor::setCache(ResponseCache *cache) { d->cache = cache; }

QStringList CachingInterceptor::cacheablePaths() const { return d->cacheablePaths; }

void CachingInterceptor::setCacheablePaths(const QStringList &paths)
{
    d->cacheablePaths = paths;
}

QByteArray CachingInterceptor::cacheKey(const InterceptedRequest &request)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    // Length-prefixed rather than concatenated, so no two different requests can
    // hash the same by having their parts run together at a different boundary.
    const auto feed = [&hash](const QByteArray &part) {
        hash.addData(QByteArray::number(part.size()));
        hash.addData(":");
        hash.addData(part);
    };

    feed(request.method);
    feed(request.request.url().toEncoded());
    feed(request.body);
    for (const char *name : kIdentityHeaders)
        feed(request.request.rawHeader(name));

    return hash.result().toHex();
}

std::optional<InterceptedResponse> CachingInterceptor::beforeRequest(InterceptedRequest &request)
{
    if (!d->isCacheable(request))
        return std::nullopt;

    const QByteArray key = cacheKey(request);
    if (const std::optional<QByteArray> body = cache()->lookup(key)) {
        Q_EMIT hit(request.url());
        InterceptedResponse answer;
        answer.body = *body;
        answer.httpStatus = 200;
        return answer;
    }

    Q_EMIT missed(request.url());
    return std::nullopt;
}

void CachingInterceptor::afterResponse(const InterceptedResponse &response)
{
    // Already served from here; storing it again would only refresh its age and
    // make an entry immortal as long as it keeps being asked for.
    if (response.fromCache)
        return;
    // An error is what the provider was doing at one moment, not a property of
    // the request. Caching one turns a blip into a sticky failure.
    if (!response.isSuccess() || response.body.isEmpty())
        return;
    if (!d->isCacheable(response.request))
        return;

    cache()->insert(cacheKey(response.request), response.body);
    Q_EMIT stored(response.url());
}

} // namespace Client
} // namespace QtOpenAi
