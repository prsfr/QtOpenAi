// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/CachingInterceptor.h>
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/ResponseCache.h>

#include <QtTest/QtTest>

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

namespace {

QByteArray completion(const QString &content)
{
    return QStringLiteral(R"({"id":"c","object":"chat.completion","created":1,
        "model":"m","choices":[{"index":0,"finish_reason":"stop",
        "message":{"role":"assistant","content":"%1"}}]})")
            .arg(content)
            .toUtf8();
}

ChatCompletionRequest ask(const QString &prompt)
{
    return ChatCompletionRequest(QStringLiteral("m"), {Message::user(prompt)});
}

// Two different answers, so a cached reply is distinguishable from a fresh one
// rather than merely plausible.
QList<StubServer::Response> twoAnswers()
{
    return {{completion(QStringLiteral("first"))}, {completion(QStringLiteral("second"))}};
}

// Counts what it is asked to do, so a test can prove the interceptor really
// went to the store the caller installed and not to its own.
class CountingCache : public ResponseCache
{
public:
    std::optional<QByteArray> lookup(const QByteArray &key) override
    {
        ++lookups;
        const auto it = entries.constFind(key);
        return it == entries.constEnd() ? std::nullopt : std::optional<QByteArray>(*it);
    }
    void insert(const QByteArray &key, const QByteArray &body) override
    {
        ++inserts;
        entries.insert(key, body);
    }
    void remove(const QByteArray &key) override { entries.remove(key); }
    void clear() override { entries.clear(); }
    int count() const override { return entries.size(); }

    QHash<QByteArray, QByteArray> entries;
    int lookups = 0;
    int inserts = 0;
};

} // namespace

// Coverage for response caching (#42).
class TestResponseCache : public QObject
{
    Q_OBJECT
private slots:
    void aSecondIdenticalRequestSkipsTheNetwork();
    void aDifferentRequestIsAMiss();
    void onlyAllowListedEndpointsAreCached();
    void getsAndDeletesAreNeverCached();
    void errorsAreNotCached();
    void differentCredentialsDoNotShareEntries();
    void streamsAreNotCached();
    void theStoreIsReplaceable();
    void theMemoryStoreEvictsOnSize();
    void theMemoryStoreExpiresOnTime();
    void keysDistinguishRequestsThatDifferAnywhere();
};

void TestResponseCache::aSecondIdenticalRequestSkipsTheNetwork()
{
    StubServer server(twoAnswers());
    Client client;
    client.setBaseUrl(server.baseUrl());

    CachingInterceptor cache;
    QSignalSpy hits(&cache, &CachingInterceptor::hit);
    QSignalSpy misses(&cache, &CachingInterceptor::missed);
    QSignalSpy stores(&cache, &CachingInterceptor::stored);
    client.addInterceptor(&cache);

    const auto first = awaited(client.createChatCompletion(ask(QStringLiteral("hello"))));
    QVERIFY(first);
    QVERIFY2(first->isSuccess(), qPrintable(first->error().message()));
    QCOMPARE(first->response().choices().at(0).message().content(), QStringLiteral("first"));
    QCOMPARE(server.requestCount(), 1);

    const auto second = awaited(client.createChatCompletion(ask(QStringLiteral("hello"))));
    QVERIFY(second);
    QVERIFY2(second->isSuccess(), qPrintable(second->error().message()));
    // The server would have answered "second" had it been asked. It was not.
    QCOMPARE(second->response().choices().at(0).message().content(), QStringLiteral("first"));
    QCOMPARE(server.requestCount(), 1);

    QCOMPARE(misses.count(), 1);
    QCOMPARE(stores.count(), 1);
    QCOMPARE(hits.count(), 1);
    QCOMPARE(cache.cache()->count(), 1);
}

void TestResponseCache::aDifferentRequestIsAMiss()
{
    StubServer server(twoAnswers());
    Client client;
    client.setBaseUrl(server.baseUrl());

    CachingInterceptor cache;
    client.addInterceptor(&cache);

    QVERIFY(awaited(client.createChatCompletion(ask(QStringLiteral("hello")))));
    const auto second = awaited(client.createChatCompletion(ask(QStringLiteral("goodbye"))));
    QVERIFY(second);
    QCOMPARE(second->response().choices().at(0).message().content(), QStringLiteral("second"));
    QCOMPARE(server.requestCount(), 2);
    QCOMPARE(cache.cache()->count(), 2);
}

void TestResponseCache::onlyAllowListedEndpointsAreCached()
{
    // Replaying a POST that *creates* something would hand back the id of an
    // object the caller believes it just made. So the default is an allow-list
    // of endpoints that change nothing, and /responses -- which stores what it
    // is given -- is deliberately not on it.
    QVERIFY(CachingInterceptor::defaultCacheablePaths().contains(
            QStringLiteral("/chat/completions")));
    QVERIFY(!CachingInterceptor::defaultCacheablePaths().contains(QStringLiteral("/responses")));

    const QByteArray response = R"({"id":"resp_1","object":"response","created_at":1,
        "model":"m","status":"completed","output":[]})";
    StubServer server(QList<StubServer::Response> {{response}, {response}});
    Client client;
    client.setBaseUrl(server.baseUrl());

    CachingInterceptor cache;
    client.addInterceptor(&cache);

    const ResponseRequest request(QStringLiteral("m"), QStringLiteral("hi"));
    QVERIFY(awaited(client.createResponse(request)));
    QVERIFY(awaited(client.createResponse(request)));
    QCOMPARE(server.requestCount(), 2);
    QCOMPARE(cache.cache()->count(), 0);

    // Extending the list is the caller's call to make, and it takes effect.
    cache.setCacheablePaths({QStringLiteral("/responses")});
    QCOMPARE(cache.cacheablePaths(), QStringList({QStringLiteral("/responses")}));
    QVERIFY(awaited(client.createResponse(request)));
    QVERIFY(awaited(client.createResponse(request)));
    QCOMPARE(server.requestCount(), 3);
}

void TestResponseCache::getsAndDeletesAreNeverCached()
{
    // A listing that cannot change is not a listing, and a cached DELETE is a
    // lie about something that did not happen.
    const QByteArray model = R"({"id":"m","object":"model","created":1,"owned_by":"me"})";
    StubServer server(QList<StubServer::Response> {{model}, {model}});
    Client client;
    client.setBaseUrl(server.baseUrl());

    CachingInterceptor cache;
    client.addInterceptor(&cache);

    QVERIFY(awaited(client.getModel(QStringLiteral("m"))));
    QVERIFY(awaited(client.getModel(QStringLiteral("m"))));
    QCOMPARE(server.requestCount(), 2);
    QCOMPARE(cache.cache()->count(), 0);
}

void TestResponseCache::errorsAreNotCached()
{
    // An error describes the provider at one moment, not the request. Storing
    // one turns a blip into a failure that never clears.
    StubServer server(QList<StubServer::Response> {
            {R"({"error":{"message":"overloaded","type":"server_error"}})", 503},
            {completion(QStringLiteral("recovered"))}});
    Client client;
    client.setBaseUrl(server.baseUrl());
    client.setRetryPolicy(RetryPolicy::none());

    CachingInterceptor cache;
    client.addInterceptor(&cache);

    const auto failed = awaited(client.createChatCompletion(ask(QStringLiteral("hello"))));
    QVERIFY(failed);
    QVERIFY(!failed->isSuccess());
    QCOMPARE(cache.cache()->count(), 0);

    const auto retried = awaited(client.createChatCompletion(ask(QStringLiteral("hello"))));
    QVERIFY(retried);
    QVERIFY2(retried->isSuccess(), qPrintable(retried->error().message()));
    QCOMPARE(retried->response().choices().at(0).message().content(), QStringLiteral("recovered"));
    QCOMPARE(server.requestCount(), 2);
}

void TestResponseCache::differentCredentialsDoNotShareEntries()
{
    // Two accounts sharing one process must not see each other's answers.
    StubServer server(twoAnswers());
    Client client;
    client.setBaseUrl(server.baseUrl());

    CachingInterceptor cache;
    client.addInterceptor(&cache);

    client.setApiKey(QStringLiteral("key-one"));
    QVERIFY(awaited(client.createChatCompletion(ask(QStringLiteral("hello")))));

    client.setApiKey(QStringLiteral("key-two"));
    const auto other = awaited(client.createChatCompletion(ask(QStringLiteral("hello"))));
    QVERIFY(other);
    QCOMPARE(other->response().choices().at(0).message().content(), QStringLiteral("second"));
    QCOMPARE(server.requestCount(), 2);
}

void TestResponseCache::streamsAreNotCached()
{
    // A stream is a sequence of events, not a body; there is nothing to store
    // and serving one from a store would mean fabricating events.
    StubServer server(QList<StubServer::Response> {{"data: [DONE]\n\n", 200, "text/event-stream"},
                                                   {"data: [DONE]\n\n", 200, "text/event-stream"}});
    Client client;
    client.setBaseUrl(server.baseUrl());

    CachingInterceptor cache;
    client.addInterceptor(&cache);

    for (int i = 0; i < 2; ++i) {
        auto *reply = client.createChatCompletionStream(ask(QStringLiteral("hello")));
        reply->setAutoDelete(false);
        QSignalSpy done(reply, &ChatCompletionStreamReply::done);
        QVERIFY(done.wait(5000));
        delete reply;
    }
    QCOMPARE(server.requestCount(), 2);
    QCOMPARE(cache.cache()->count(), 0);
}

void TestResponseCache::theStoreIsReplaceable()
{
    // The in-memory default is a default, not the design: a process that wants
    // its cache on disk or shared between instances supplies its own.
    CountingCache store;
    StubServer server(twoAnswers());
    Client client;
    client.setBaseUrl(server.baseUrl());

    CachingInterceptor cache;
    cache.setCache(&store);
    QCOMPARE(cache.cache(), &store);
    client.addInterceptor(&cache);

    QVERIFY(awaited(client.createChatCompletion(ask(QStringLiteral("hello")))));
    QVERIFY(awaited(client.createChatCompletion(ask(QStringLiteral("hello")))));
    QCOMPARE(store.lookups, 2);
    QCOMPARE(store.inserts, 1);
    QCOMPARE(store.count(), 1);
    QCOMPARE(server.requestCount(), 1);

    // Handing back nullptr restores the built-in one rather than leaving the
    // interceptor pointing at a store the caller is about to destroy.
    cache.setCache(nullptr);
    QVERIFY(cache.cache() != &store);
    QCOMPARE(cache.cache()->count(), 0);
}

void TestResponseCache::theMemoryStoreEvictsOnSize()
{
    // Without a ceiling, a long-running process that varies its prompts grows
    // until it is killed.
    MemoryResponseCache store(2);
    QCOMPARE(store.maxEntries(), 2);

    store.insert("a", "1");
    store.insert("b", "2");
    QCOMPARE(store.count(), 2);

    store.insert("c", "3");
    QCOMPARE(store.count(), 2);
    QVERIFY(store.lookup("c").has_value());
    QVERIFY(!store.lookup("a").has_value());

    store.remove("c");
    QVERIFY(!store.lookup("c").has_value());
    store.clear();
    QCOMPARE(store.count(), 0);

    // A store with no room stores nothing rather than misreporting a hit.
    store.setMaxEntries(0);
    store.insert("d", "4");
    QCOMPARE(store.count(), 0);
}

void TestResponseCache::theMemoryStoreExpiresOnTime()
{
    MemoryResponseCache store;
    QCOMPARE(store.ttlSeconds(), 300);

    store.setTtlSeconds(1);
    store.insert("a", "1");
    QCOMPARE(store.lookup("a").value(), QByteArray("1"));

    QTest::qWait(1100);
    // A cached answer outlives the question, so an expired entry is a miss ...
    QVERIFY(!store.lookup("a").has_value());
    // ... and is dropped rather than left occupying a slot.
    QCOMPARE(store.count(), 0);

    // 0 means "never", for a caller who wants only the size limit.
    store.setTtlSeconds(0);
    store.insert("b", "2");
    QTest::qWait(50);
    QVERIFY(store.lookup("b").has_value());
}

void TestResponseCache::keysDistinguishRequestsThatDifferAnywhere()
{
    const auto keyFor = [](const QByteArray &method, const QString &url, const QByteArray &body,
                           const QByteArray &auth) {
        InterceptedRequest request;
        request.method = method;
        request.request.setUrl(QUrl(url));
        request.body = body;
        if (!auth.isEmpty())
            request.request.setRawHeader("Authorization", auth);
        return CachingInterceptor::cacheKey(request);
    };

    const QByteArray base
            = keyFor("POST", QStringLiteral("https://h/v1/embeddings"), "{\"a\":1}", "Bearer one");
    QCOMPARE(base,
             keyFor("POST", QStringLiteral("https://h/v1/embeddings"), "{\"a\":1}", "Bearer one"));

    QVERIFY(base
            != keyFor("GET", QStringLiteral("https://h/v1/embeddings"), "{\"a\":1}", "Bearer one"));
    QVERIFY(base
            != keyFor("POST", QStringLiteral("https://h/v1/moderations"), "{\"a\":1}",
                      "Bearer one"));
    QVERIFY(base
            != keyFor("POST", QStringLiteral("https://h/v1/embeddings"), "{\"a\":2}",
                      "Bearer one"));
    QVERIFY(base
            != keyFor("POST", QStringLiteral("https://h/v1/embeddings"), "{\"a\":1}",
                      "Bearer two"));

    // Length-prefixed, so parts cannot run together into a collision: "ab" + ""
    // and "a" + "b" are different requests and must be different keys.
    QVERIFY(keyFor("POST", QStringLiteral("https://h/ab"), {}, {})
            != keyFor("POST", QStringLiteral("https://h/a"), "b", {}));

    // The credential is hashed, never stored: the key must not contain it.
    QVERIFY(!base.contains("one"));
}

QTEST_MAIN(TestResponseCache)
#include "tst_responsecache.moc"
