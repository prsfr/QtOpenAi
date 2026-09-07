// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/CachingInterceptor.h>
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Storage/JsonFileStore.h>
#include <QtOpenAi/Storage/PersistentResponseCache.h>

#include <QtCore/QTemporaryDir>
#include <QtTest/QtTest>

#include "support/AwaitedReply.h"
#include "support/BatchCountingStore.h"
#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;
using namespace QtOpenAi::Storage;

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

} // namespace

// Coverage for the store-backed response cache (#48, over #42).
class TestPersistentCache : public QObject
{
    Q_OBJECT
private slots:
    void aCachedAnswerSurvivesTheProcessThatCachedIt();
    void expiredEntriesAreMissesAndAreDropped();
    void theCeilingEvictsTheOldest();
    void oneInsertIsOneBatch();
    void removeAndClearReachTheStore();
    void aCacheWithoutAStoreIsJustAMiss();
};

void TestPersistentCache::aCachedAnswerSurvivesTheProcessThatCachedIt()
{
    // The whole point of the layer: the second *session* skips the network,
    // not merely the second request.
    QTemporaryDir root;
    StubServer server(twoAnswers());

    {
        JsonFileStore store(root.path());
        QVERIFY2(store.open(), qPrintable(store.lastError()));
        PersistentResponseCache cache(&store);

        Client client;
        client.setBaseUrl(server.baseUrl());
        CachingInterceptor interceptor;
        interceptor.setCache(&cache);
        client.addInterceptor(&interceptor);

        const auto first = awaited(client.createChatCompletion(ask(QStringLiteral("hello"))));
        QVERIFY(first);
        QVERIFY2(first->isSuccess(), qPrintable(first->error().message()));
        QCOMPARE(server.requestCount(), 1);
        QCOMPARE(cache.count(), 1);
    }

    // A second store over the same directory, a second client, a second
    // interceptor: everything the first session had is gone but the disk.
    JsonFileStore reopened(root.path());
    QVERIFY2(reopened.open(), qPrintable(reopened.lastError()));
    PersistentResponseCache cache(&reopened);

    Client client;
    client.setBaseUrl(server.baseUrl());
    CachingInterceptor interceptor;
    interceptor.setCache(&cache);
    QSignalSpy hits(&interceptor, &CachingInterceptor::hit);
    client.addInterceptor(&interceptor);

    const auto second = awaited(client.createChatCompletion(ask(QStringLiteral("hello"))));
    QVERIFY(second);
    QVERIFY2(second->isSuccess(), qPrintable(second->error().message()));
    // The server would have answered "second" had it been asked. It was not.
    QCOMPARE(second->response().choices().at(0).message().content(), QStringLiteral("first"));
    QCOMPARE(server.requestCount(), 1);
    QCOMPARE(hits.count(), 1);
}

void TestPersistentCache::expiredEntriesAreMissesAndAreDropped()
{
    QTemporaryDir root;
    JsonFileStore store(root.path());
    QVERIFY2(store.open(), qPrintable(store.lastError()));

    PersistentResponseCache cache(&store);
    cache.setTtlSeconds(1);
    cache.insert("key", "body");
    QCOMPARE(cache.count(), 1);

    // Backdate the entry rather than waiting for it: the store is where the
    // timestamp lives, so putting an old one there is the same event.
    CachedResponse aged;
    aged.key = "key";
    aged.body = "body";
    aged.storedAt = QDateTime::currentDateTimeUtc().addSecs(-60);
    QVERIFY(store.saveCachedResponse(aged));

    QVERIFY(!cache.lookup("key").has_value());
    // Dropped on the miss, not left to occupy a live slot.
    QCOMPARE(cache.count(), 0);

    cache.setTtlSeconds(0); // expiry off
    QVERIFY(store.saveCachedResponse(aged));
    const std::optional<QByteArray> found = cache.lookup("key");
    QVERIFY(found.has_value());
    QCOMPARE(*found, QByteArray("body"));
}

void TestPersistentCache::theCeilingEvictsTheOldest()
{
    QTemporaryDir root;
    JsonFileStore store(root.path());
    QVERIFY2(store.open(), qPrintable(store.lastError()));

    PersistentResponseCache cache(&store);
    cache.setMaxEntries(2);
    QCOMPARE(cache.maxEntries(), 2);

    cache.insert("a", "1");
    QTest::qWait(5); // millisecond timestamps: without this the three would tie
    cache.insert("b", "2");
    QTest::qWait(5);
    cache.insert("c", "3");

    QCOMPARE(cache.count(), 2);
    QVERIFY(!cache.lookup("a").has_value());
    QVERIFY(cache.lookup("b").has_value());
    QVERIFY(cache.lookup("c").has_value());

    // Zero means "store nothing", the same as MemoryResponseCache -- one
    // spelling for off beats two.
    cache.setMaxEntries(0);
    cache.insert("d", "4");
    QVERIFY(!cache.lookup("d").has_value());
}

void TestPersistentCache::oneInsertIsOneBatch()
{
    // An insert is a save plus the prune it triggers, and every cached
    // response pays for the pair. The store sees them as one batch, which is
    // what makes them one commit on a backend with transactions.
    QTemporaryDir root;
    BatchCountingStore store(root.path());
    QVERIFY2(store.open(), qPrintable(store.lastError()));

    PersistentResponseCache cache(&store);
    cache.setMaxEntries(1);
    cache.insert("a", "1");
    QCOMPARE(store.batchesBegun, 1);
    QCOMPARE(store.batchesEnded, 1);
    QCOMPARE(store.batchesDropped, 0);
    QVERIFY(cache.lookup("a").has_value());

    // A cache that stores nothing writes nothing, so there is no batch for it
    // to write nothing in.
    cache.setMaxEntries(0);
    cache.insert("b", "2");
    QCOMPARE(store.batchesBegun, 1);
}

void TestPersistentCache::removeAndClearReachTheStore()
{
    QTemporaryDir root;
    JsonFileStore store(root.path());
    QVERIFY2(store.open(), qPrintable(store.lastError()));

    PersistentResponseCache cache(&store);
    QCOMPARE(cache.store(), &store);
    cache.insert("a", "1");
    cache.insert("b", "2");

    cache.remove("a");
    QVERIFY(!store.cachedResponse("a").has_value());
    QCOMPARE(cache.count(), 1);

    cache.clear();
    QCOMPARE(store.cachedResponseCount(), 0);
}

void TestPersistentCache::aCacheWithoutAStoreIsJustAMiss()
{
    // Constructing one before the store exists must not crash the caller that
    // then uses it -- an application whose store failed to open still runs.
    PersistentResponseCache cache(nullptr);
    cache.insert("a", "1");
    QVERIFY(!cache.lookup("a").has_value());
    QCOMPARE(cache.count(), 0);
    cache.remove("a");
    cache.clear();
}

QTEST_MAIN(TestPersistentCache)
#include "tst_persistentcache.moc"
