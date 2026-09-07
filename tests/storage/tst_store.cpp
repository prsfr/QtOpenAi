// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Metrics.h>
#include <QtOpenAi/Storage/JsonFileStore.h>
#include <QtOpenAi/Storage/Store.h>

#ifdef QTOPENAI_TESTS_HAVE_SQL
#include <QtOpenAi/Sql/SqliteStore.h>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#endif

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Chat;
using namespace QtOpenAi::Client;
using namespace QtOpenAi::Storage;

namespace {

// Every test here runs against every backend, because "the backends are
// swappable" is a claim about them behaving alike and not about each one
// working on its own. The row name is also the column, so a failure names the
// backend it happened on.
void addBackends()
{
    QTest::addColumn<QString>("backend");
    QTest::newRow("json-files") << QStringLiteral("json-files");
#ifdef QTOPENAI_TESTS_HAVE_SQL
    QTest::newRow("sqlite") << QStringLiteral("sqlite");
#endif
}

std::unique_ptr<Store> makeStore(const QString &backend, const QString &root)
{
#ifdef QTOPENAI_TESTS_HAVE_SQL
    if (backend == QLatin1String("sqlite"))
        return std::make_unique<QtOpenAi::Sql::SqliteStore>(root + QStringLiteral("/history.db"));
#endif
    Q_UNUSED(backend);
    return std::make_unique<JsonFileStore>(root + QStringLiteral("/history"));
}

// A conversation that branched: the model was asked again, so the first answer
// and the second are both in the tree and only one is on the active path.
Transcript branchedTranscript(Transcript::NodeId *firstAnswer = nullptr,
                              Transcript::NodeId *secondAnswer = nullptr)
{
    Transcript transcript;
    transcript.setSystemPrompt(QStringLiteral("be terse"));
    transcript.addUserMessage(QStringLiteral("why is the sky blue?"));
    const Transcript::NodeId first = transcript.addMessage(
            Message(Role::Assistant, QStringLiteral("rayleigh scattering")));
    const Transcript::NodeId second
            = transcript.fork(first, Message(Role::Assistant, QStringLiteral("short wavelengths")));
    if (firstAnswer)
        *firstAnswer = first;
    if (secondAnswer)
        *secondAnswer = second;
    return transcript;
}

MetricsSnapshot recordedSnapshot()
{
    MetricsSnapshot snapshot;
    snapshot.requests = 7;
    snapshot.successes = 5;
    snapshot.failures = 2;
    snapshot.failuresByStatus.insert(429, 1);
    snapshot.failuresByStatus.insert(0, 1);
    snapshot.totalDurationMs = 4200;
    snapshot.slowestDurationMs = 1800;
    snapshot.streamedRequests = 2;
    snapshot.totalTimeToFirstTokenMs = 260;

    ModelMetrics mini;
    mini.requests = 4;
    mini.promptTokens = 1200;
    mini.completionTokens = 340;
    mini.totalTokens = 1540;
    mini.cost = 0.0031;
    snapshot.models.insert(QStringLiteral("gpt-4o-mini"), mini);

    snapshot.rateLimit.limitRequests = 500;
    snapshot.rateLimit.remainingRequests = 499;
    snapshot.rateLimit.retryAfterMs = 1200;
    return snapshot;
}

// The ids of a listing, in the order it gave them: what the bounded-listing
// assertions are about, without a record's other four fields in the way.
QStringList idsOf(const QList<ConversationRecord> &records)
{
    QStringList ids;
    ids.reserve(records.size());
    for (const ConversationRecord &record : records)
        ids.append(record.id);
    return ids;
}

// `count` one-message conversations, saved a few milliseconds apart so that
// the listing order is the store's rather than a tie broken by id. Returns
// them newest first, which is the order a listing owes -- or an empty list
// when a save failed, which the caller sees as a size mismatch.
QStringList spacedConversations(Store *store, int count)
{
    Transcript transcript;
    transcript.addUserMessage(QStringLiteral("hello"));

    QStringList newestFirst;
    for (int i = 0; i < count; ++i) {
        if (i > 0)
            QTest::qWait(5);
        const QString id = QStringLiteral("c%1").arg(i);
        if (!store->saveConversation(id, transcript, id))
            return {};
        newestFirst.prepend(id);
    }
    return newestFirst;
}

// Whether the backend can drop what a batch already wrote. SQLite has a
// transaction to roll back; a directory of QSaveFiles has nothing to undo, and
// the interface says as much -- a batch is a hint about grouping, never a
// promise of atomicity. Asserting the same outcome for both would be asserting
// something only one of them ever agreed to.
bool dropsWhatABatchWrote(const QString &backend)
{
#ifdef QTOPENAI_TESTS_HAVE_SQL
    return backend == QLatin1String("sqlite");
#else
    Q_UNUSED(backend);
    return false;
#endif
}

CachedResponse entry(const QByteArray &key, const QByteArray &body, const QDateTime &storedAt)
{
    CachedResponse response;
    response.key = key;
    response.body = body;
    response.storedAt = storedAt;
    return response;
}

// Make the store look like one a newer build wrote, without going through its
// own API -- which has no way to write a version it does not support, and
// should not grow one for a test.
void stampSchemaVersion(const QString &backend, const QString &root, int version)
{
#ifdef QTOPENAI_TESTS_HAVE_SQL
    if (backend == QLatin1String("sqlite")) {
        const QString connection = QStringLiteral("tst_store_stamp");
        {
            QSqlDatabase database
                    = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
            database.setDatabaseName(root + QStringLiteral("/history.db"));
            QVERIFY(database.open());
            QSqlQuery query(database);
            QVERIFY(query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS meta "
                                              "(key TEXT PRIMARY KEY, value TEXT NOT NULL)")));
            QVERIFY(query.prepare(QStringLiteral("INSERT OR REPLACE INTO meta (key, value) "
                                                 "VALUES ('schema_version', ?)")));
            query.addBindValue(QString::number(version));
            QVERIFY(query.exec());
            database.close();
        }
        QSqlDatabase::removeDatabase(connection);
        return;
    }
#endif
    Q_UNUSED(backend);
    const QString directory = root + QStringLiteral("/history");
    QVERIFY(QDir().mkpath(directory));
    QFile file(directory + QStringLiteral("/store.json"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QJsonDocument(QJsonObject {{QLatin1String("schema_version"), version}}).toJson());
}

} // namespace

// Coverage for the persistence layer (#48). Offline throughout: a store is a
// temporary directory or a temporary database file, and nothing here opens a
// socket.
class TestStore : public QObject
{
    Q_OBJECT
private slots:
    void aFreshStoreReportsTheCurrentSchemaVersion();
    void aFreshStoreReportsTheCurrentSchemaVersion_data() { addBackends(); }

    void aBranchedConversationSurvivesSaveAndLoad();
    void aBranchedConversationSurvivesSaveAndLoad_data() { addBackends(); }

    void listsConversationsMostRecentlyUpdatedFirst();
    void listsConversationsMostRecentlyUpdatedFirst_data() { addBackends(); }

    void listsABoundedPageOfConversations();
    void listsABoundedPageOfConversations_data() { addBackends(); }

    void writesInABatchLandTogether();
    void writesInABatchLandTogether_data() { addBackends(); }

    void nestedBatchesEndWithTheOutermost();
    void nestedBatchesEndWithTheOutermost_data() { addBackends(); }

    void anAbortedBatchIsDroppedWhereTheBackendCan();
    void anAbortedBatchIsDroppedWhereTheBackendCan_data() { addBackends(); }

    void anEmptyTitleKeepsTheStoredOne();
    void anEmptyTitleKeepsTheStoredOne_data() { addBackends(); }

    void aConversationThatIsNotThereIsNotAnError();
    void aConversationThatIsNotThereIsNotAnError_data() { addBackends(); }

    void removesAConversation();
    void removesAConversation_data() { addBackends(); }

    void cachedResponsesRoundTrip();
    void cachedResponsesRoundTrip_data() { addBackends(); }

    void pruningDropsTheOldestBeyondTheCeiling();
    void pruningDropsTheOldestBeyondTheCeiling_data() { addBackends(); }

    void pruningDropsEverythingOlderThanTheCutoff();
    void pruningDropsEverythingOlderThanTheCutoff_data() { addBackends(); }

    void aMetricsSnapshotSurvivesSaveAndLoad();
    void aMetricsSnapshotSurvivesSaveAndLoad_data() { addBackends(); }

    void listsAndRemovesMetrics();
    void listsAndRemovesMetrics_data() { addBackends(); }

    void refusesAStoreWrittenByANewerVersion();
    void refusesAStoreWrittenByANewerVersion_data() { addBackends(); }

    void aClosedStoreFailsRatherThanPretending();
    void aClosedStoreFailsRatherThanPretending_data() { addBackends(); }
};

void TestStore::aFreshStoreReportsTheCurrentSchemaVersion()
{
    QFETCH(QString, backend);
    QTemporaryDir root;
    const auto store = makeStore(backend, root.path());

    QVERIFY2(store->open(), qPrintable(store->lastError()));
    QVERIFY(store->isOpen());
    QCOMPARE(store->schemaVersion(), Store::CurrentSchemaVersion);
    QVERIFY(store->lastError().isEmpty());

    // Opening an open store is a no-op rather than a second store.
    QVERIFY(store->open());
    QCOMPARE(store->conversations().size(), 0);
}

void TestStore::aBranchedConversationSurvivesSaveAndLoad()
{
    // The acceptance criterion: a conversation *with a branch* round-trips
    // unchanged. A store that kept only the active path would pass every other
    // test in this file and still lose the user's other answer.
    QFETCH(QString, backend);
    QTemporaryDir root;
    Transcript::NodeId firstAnswer = Transcript::InvalidNode;
    Transcript::NodeId secondAnswer = Transcript::InvalidNode;
    const Transcript original = branchedTranscript(&firstAnswer, &secondAnswer);

    {
        const auto store = makeStore(backend, root.path());
        QVERIFY2(store->open(), qPrintable(store->lastError()));
        QVERIFY(store->saveConversation(QStringLiteral("chat-1"), original,
                                        QStringLiteral("Sky colours")));
    }

    // A second store over the same path is the next run of the application.
    const auto reopened = makeStore(backend, root.path());
    QVERIFY2(reopened->open(), qPrintable(reopened->lastError()));

    const std::optional<Transcript> loaded = reopened->loadConversation(QStringLiteral("chat-1"));
    QVERIFY(loaded.has_value());
    QCOMPARE(*loaded, original);
    QCOMPARE(loaded->systemPrompt(), QStringLiteral("be terse"));
    QCOMPARE(loaded->activeLeaf(), secondAnswer);
    // Both answers are still reachable, which is what "with a branch" means.
    QCOMPARE(loaded->siblings(secondAnswer),
             QList<Transcript::NodeId>({firstAnswer, secondAnswer}));
    QCOMPARE(loaded->message(firstAnswer).content(), QStringLiteral("rayleigh scattering"));

    const std::optional<ConversationRecord> record
            = reopened->conversation(QStringLiteral("chat-1"));
    QVERIFY(record.has_value());
    QCOMPARE(record->id, QStringLiteral("chat-1"));
    QCOMPARE(record->title, QStringLiteral("Sky colours"));
    // Three nodes: the question and both answers. The branch counts.
    QCOMPARE(record->messageCount, 3);
    QVERIFY(record->createdAt.isValid());
    QVERIFY(record->updatedAt >= record->createdAt);
}

void TestStore::listsConversationsMostRecentlyUpdatedFirst()
{
    QFETCH(QString, backend);
    QTemporaryDir root;
    const auto store = makeStore(backend, root.path());
    QVERIFY2(store->open(), qPrintable(store->lastError()));

    Transcript one;
    one.addUserMessage(QStringLiteral("first"));
    QVERIFY(store->saveConversation(QStringLiteral("older"), one, QStringLiteral("Older")));
    // Both backends keep millisecond timestamps, so two saves in the same
    // millisecond would tie; the wait makes the order the test's subject
    // rather than the scheduler's.
    QTest::qWait(5);
    Transcript two;
    two.addUserMessage(QStringLiteral("second"));
    QVERIFY(store->saveConversation(QStringLiteral("newer"), two, QStringLiteral("Newer")));

    const QList<ConversationRecord> records = store->conversations();
    QCOMPARE(records.size(), 2);
    QCOMPARE(records.at(0).id, QStringLiteral("newer"));
    QCOMPARE(records.at(1).id, QStringLiteral("older"));
    QCOMPARE(records.at(0).title, QStringLiteral("Newer"));
    QCOMPARE(records.at(1).messageCount, 1);
}

void TestStore::listsABoundedPageOfConversations()
{
    // What a conversation sidebar actually asks for: the newest page, and the
    // one after it. The unbounded listing is the same call without a bound,
    // rather than a second thing the backends have to keep in step.
    QFETCH(QString, backend);
    QTemporaryDir root;
    const auto store = makeStore(backend, root.path());
    QVERIFY2(store->open(), qPrintable(store->lastError()));

    const QStringList newestFirst = spacedConversations(store.get(), 5);
    QCOMPARE(newestFirst.size(), 5);

    QCOMPARE(idsOf(store->conversations(2)), newestFirst.mid(0, 2));
    QCOMPARE(idsOf(store->conversations(2, 2)), newestFirst.mid(2, 2));
    // A page that runs past the end is short rather than an error, and one
    // that starts past it is empty.
    QCOMPARE(idsOf(store->conversations(2, 4)), newestFirst.mid(4, 1));
    QCOMPARE(store->conversations(2, 5).size(), 0);
    // A negative limit is no limit, which is what conversations() is; zero is
    // an empty listing; a negative offset is the first page.
    QCOMPARE(idsOf(store->conversations(-1)), newestFirst);
    QCOMPARE(idsOf(store->conversations()), newestFirst);
    QCOMPARE(store->conversations(0).size(), 0);
    QCOMPARE(idsOf(store->conversations(2, -3)), newestFirst.mid(0, 2));

    // Whole records, not ids with the rest left out: a bounded listing is the
    // listing, and an application draws its rows from exactly these fields.
    const QList<ConversationRecord> page = store->conversations(1);
    QCOMPARE(page.size(), 1);
    QCOMPARE(page.at(0).title, newestFirst.at(0));
    QCOMPARE(page.at(0).messageCount, 1);
    QVERIFY(page.at(0).createdAt.isValid());
    QVERIFY(page.at(0).updatedAt >= page.at(0).createdAt);
    // A listing is not a failure, whichever bound it was given.
    QVERIFY(store->lastError().isEmpty());

    // Reachable through a concrete backend and not only through a Store *:
    // a class that overrides one of the two overloads hides the other, which
    // is what the using declaration in JsonFileStore is there to prevent. The
    // assertion is the compiler's; the row it runs in does not matter.
    JsonFileStore concrete(root.path() + QStringLiteral("/direct"));
    QVERIFY2(concrete.open(), qPrintable(concrete.lastError()));
    QVERIFY(concrete.conversations(50).isEmpty());
}

void TestStore::writesInABatchLandTogether()
{
    // Batching is a hint a backend may ignore, so what every backend owes is
    // this: the writes inside one are in the store afterwards, and the pair of
    // calls succeeds whether or not it grouped anything.
    QFETCH(QString, backend);
    QTemporaryDir root;
    const auto store = makeStore(backend, root.path());
    QVERIFY2(store->open(), qPrintable(store->lastError()));

    Transcript transcript;
    transcript.addUserMessage(QStringLiteral("hello"));

    QVERIFY(store->beginBatch());
    QVERIFY(store->saveConversation(QStringLiteral("a"), transcript, QStringLiteral("A")));
    QVERIFY(store->saveMetrics(QStringLiteral("all-time"), recordedSnapshot()));
    QVERIFY(store->saveCachedResponse(entry("k", "1", QDateTime::currentDateTimeUtc())));
    QVERIFY(store->endBatch());

    QCOMPARE(idsOf(store->conversations()), QStringList({QStringLiteral("a")}));
    QVERIFY(store->loadMetrics(QStringLiteral("all-time")).has_value());
    QVERIFY(store->cachedResponse("k").has_value());

    // The guard is those two calls with the scope for a caller: what it wrote
    // is there once the scope closed.
    {
        Store::Batch batch(store.get());
        QVERIFY(batch.isActive());
        QVERIFY(store->saveConversation(QStringLiteral("b"), transcript, QStringLiteral("B")));
    }
    QVERIFY(store->conversation(QStringLiteral("b")).has_value());

    Store::Batch ended(store.get());
    QVERIFY(store->saveConversation(QStringLiteral("c"), transcript, QStringLiteral("C")));
    QVERIFY(ended.commit());
    // Ended by hand, so the destructor has nothing left to end.
    QVERIFY(!ended.isActive());
    QCOMPARE(store->conversations().size(), 3);
}

void TestStore::nestedBatchesEndWithTheOutermost()
{
    // The library batches its own writes -- a cache insert and the prune it
    // triggers -- inside whatever the application opened, so an inner pair has
    // to be counted rather than taken for the whole batch.
    QFETCH(QString, backend);
    QTemporaryDir root;
    const auto store = makeStore(backend, root.path());
    QVERIFY2(store->open(), qPrintable(store->lastError()));

    Transcript transcript;
    transcript.addUserMessage(QStringLiteral("hello"));

    QVERIFY(store->beginBatch());
    QVERIFY(store->saveConversation(QStringLiteral("outer"), transcript));
    QVERIFY(store->beginBatch());
    QVERIFY(store->saveConversation(QStringLiteral("inner"), transcript));
    QVERIFY(store->endBatch()); // the inner one: the batch is still open
    QVERIFY(store->saveConversation(QStringLiteral("after"), transcript));
    QVERIFY(store->endBatch()); // the outermost one is what settles all three
    QCOMPARE(store->conversations().size(), 3);

    // An inner drop takes the whole batch with it: an inner caller that could
    // not finish must not have its half kept by an outer one that cannot know.
    QVERIFY(store->beginBatch());
    QVERIFY(store->saveConversation(QStringLiteral("dropped"), transcript));
    QVERIFY(store->beginBatch());
    QVERIFY(store->endBatch(false));
    QVERIFY(store->endBatch());
    QCOMPARE(store->conversation(QStringLiteral("dropped")).has_value(),
             !dropsWhatABatchWrote(backend));
}

void TestStore::anAbortedBatchIsDroppedWhereTheBackendCan()
{
    QFETCH(QString, backend);
    QTemporaryDir root;
    const auto store = makeStore(backend, root.path());
    QVERIFY2(store->open(), qPrintable(store->lastError()));

    Transcript transcript;
    transcript.addUserMessage(QStringLiteral("hello"));

    QVERIFY(store->beginBatch());
    QVERIFY(store->saveConversation(QStringLiteral("a"), transcript));
    QVERIFY(store->endBatch(false));
    QCOMPARE(store->conversation(QStringLiteral("a")).has_value(), !dropsWhatABatchWrote(backend));

    // Either way the store is usable straight after, and the next batch is a
    // new one rather than the dropped one carried on.
    Store::Batch batch(store.get());
    QVERIFY(store->saveConversation(QStringLiteral("b"), transcript));
    QVERIFY(batch.commit());
    QVERIFY(store->conversation(QStringLiteral("b")).has_value());
}

void TestStore::anEmptyTitleKeepsTheStoredOne()
{
    // Autosave has no title to pass, and must not blank the one the user set.
    QFETCH(QString, backend);
    QTemporaryDir root;
    const auto store = makeStore(backend, root.path());
    QVERIFY2(store->open(), qPrintable(store->lastError()));

    Transcript transcript;
    transcript.addUserMessage(QStringLiteral("hello"));
    QVERIFY(store->saveConversation(QStringLiteral("c"), transcript, QStringLiteral("Named")));

    transcript.addMessage(Message(Role::Assistant, QStringLiteral("hi")));
    QVERIFY(store->saveConversation(QStringLiteral("c"), transcript));

    const std::optional<ConversationRecord> record = store->conversation(QStringLiteral("c"));
    QVERIFY(record.has_value());
    QCOMPARE(record->title, QStringLiteral("Named"));
    QCOMPARE(record->messageCount, 2);
}

void TestStore::aConversationThatIsNotThereIsNotAnError()
{
    QFETCH(QString, backend);
    QTemporaryDir root;
    const auto store = makeStore(backend, root.path());
    QVERIFY2(store->open(), qPrintable(store->lastError()));

    QVERIFY(!store->loadConversation(QStringLiteral("ghost")).has_value());
    QVERIFY(!store->conversation(QStringLiteral("ghost")).has_value());
    // A miss is a miss, not a failure: lastError() is for the disk going away,
    // and a caller checking it must not find the last lookup in there instead.
    QVERIFY(store->lastError().isEmpty());
}

void TestStore::removesAConversation()
{
    QFETCH(QString, backend);
    QTemporaryDir root;
    const auto store = makeStore(backend, root.path());
    QVERIFY2(store->open(), qPrintable(store->lastError()));

    Transcript transcript;
    transcript.addUserMessage(QStringLiteral("hello"));
    QVERIFY(store->saveConversation(QStringLiteral("c"), transcript));
    QVERIFY(store->removeConversation(QStringLiteral("c")));

    QVERIFY(!store->loadConversation(QStringLiteral("c")).has_value());
    QCOMPARE(store->conversations().size(), 0);
    // Removing what is not there is the state the caller asked for.
    QVERIFY(store->removeConversation(QStringLiteral("c")));
}

void TestStore::cachedResponsesRoundTrip()
{
    QFETCH(QString, backend);
    QTemporaryDir root;
    const QDateTime now = QDateTime::currentDateTimeUtc();
    // Bytes, not text: a body may be a JPEG or a gzip stream, and a store that
    // round-trips only valid UTF-8 would corrupt both.
    const QByteArray body = QByteArray::fromHex("0001ff") + R"({"ok":true})";

    {
        const auto store = makeStore(backend, root.path());
        QVERIFY2(store->open(), qPrintable(store->lastError()));
        QVERIFY(store->saveCachedResponse(entry("key-a", body, now)));
        QVERIFY(store->saveCachedResponse(entry("key-b", "second", now)));
        QCOMPARE(store->cachedResponseCount(), 2);
    }

    const auto reopened = makeStore(backend, root.path());
    QVERIFY2(reopened->open(), qPrintable(reopened->lastError()));
    QCOMPARE(reopened->cachedResponseCount(), 2);

    const std::optional<CachedResponse> found = reopened->cachedResponse("key-a");
    QVERIFY(found.has_value());
    QCOMPARE(found->body, body);
    QCOMPARE(found->storedAt.toMSecsSinceEpoch(), now.toMSecsSinceEpoch());

    QVERIFY(!reopened->cachedResponse("key-missing").has_value());
    QVERIFY(reopened->removeCachedResponse("key-a"));
    QVERIFY(!reopened->cachedResponse("key-a").has_value());
    QCOMPARE(reopened->cachedResponseCount(), 1);

    QVERIFY(reopened->clearCachedResponses());
    QCOMPARE(reopened->cachedResponseCount(), 0);
}

void TestStore::pruningDropsTheOldestBeyondTheCeiling()
{
    QFETCH(QString, backend);
    QTemporaryDir root;
    const auto store = makeStore(backend, root.path());
    QVERIFY2(store->open(), qPrintable(store->lastError()));

    const QDateTime now = QDateTime::currentDateTimeUtc();
    QVERIFY(store->saveCachedResponse(entry("old", "1", now.addSecs(-300))));
    QVERIFY(store->saveCachedResponse(entry("middle", "2", now.addSecs(-200))));
    QVERIFY(store->saveCachedResponse(entry("new", "3", now.addSecs(-100))));

    QVERIFY(store->pruneCachedResponses(2, QDateTime()));

    QCOMPARE(store->cachedResponseCount(), 2);
    QVERIFY(!store->cachedResponse("old").has_value());
    QVERIFY(store->cachedResponse("middle").has_value());
    QVERIFY(store->cachedResponse("new").has_value());

    // A negative ceiling is no ceiling and an invalid cutoff no cutoff, so
    // this prune has nothing to do.
    QVERIFY(store->pruneCachedResponses(-1, QDateTime()));
    QCOMPARE(store->cachedResponseCount(), 2);
}

void TestStore::pruningDropsEverythingOlderThanTheCutoff()
{
    QFETCH(QString, backend);
    QTemporaryDir root;
    const auto store = makeStore(backend, root.path());
    QVERIFY2(store->open(), qPrintable(store->lastError()));

    const QDateTime now = QDateTime::currentDateTimeUtc();
    QVERIFY(store->saveCachedResponse(entry("stale", "1", now.addSecs(-3600))));
    QVERIFY(store->saveCachedResponse(entry("fresh", "2", now)));

    QVERIFY(store->pruneCachedResponses(-1, now.addSecs(-60)));

    QCOMPARE(store->cachedResponseCount(), 1);
    QVERIFY(!store->cachedResponse("stale").has_value());
    QVERIFY(store->cachedResponse("fresh").has_value());
}

void TestStore::aMetricsSnapshotSurvivesSaveAndLoad()
{
    QFETCH(QString, backend);
    QTemporaryDir root;
    const MetricsSnapshot original = recordedSnapshot();

    {
        const auto store = makeStore(backend, root.path());
        QVERIFY2(store->open(), qPrintable(store->lastError()));
        QVERIFY(store->saveMetrics(QStringLiteral("all-time"), original));
    }

    const auto reopened = makeStore(backend, root.path());
    QVERIFY2(reopened->open(), qPrintable(reopened->lastError()));

    const std::optional<MetricsSnapshot> loaded = reopened->loadMetrics(QStringLiteral("all-time"));
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->requests, original.requests);
    QCOMPARE(loaded->successes, original.successes);
    QCOMPARE(loaded->failures, original.failures);
    QVERIFY(loaded->failuresByStatus == original.failuresByStatus);
    QCOMPARE(loaded->totalDurationMs, original.totalDurationMs);
    QCOMPARE(loaded->slowestDurationMs, original.slowestDurationMs);
    QCOMPARE(loaded->streamedRequests, original.streamedRequests);
    QCOMPARE(loaded->totalTimeToFirstTokenMs, original.totalTimeToFirstTokenMs);
    QCOMPARE(loaded->models.size(), 1);

    const ModelMetrics mini = loaded->models.value(QStringLiteral("gpt-4o-mini"));
    QCOMPARE(mini.requests, 4);
    QCOMPARE(mini.promptTokens, qint64(1200));
    QCOMPARE(mini.completionTokens, qint64(340));
    QCOMPARE(mini.totalTokens, qint64(1540));
    QCOMPARE(mini.cost, 0.0031);
    // Derived on the way out rather than stored, so it has to come back right.
    QCOMPARE(loaded->cost(), 0.0031);
    QCOMPARE(loaded->averageDurationMs(), 600.0);

    QCOMPARE(loaded->rateLimit.limitRequests, 500);
    QCOMPARE(loaded->rateLimit.remainingRequests, 499);
    QCOMPARE(loaded->rateLimit.retryAfterMs, 1200);
    // Headers the provider never sent stay absent rather than becoming zero.
    QCOMPARE(loaded->rateLimit.limitTokens, -1);
}

void TestStore::listsAndRemovesMetrics()
{
    QFETCH(QString, backend);
    QTemporaryDir root;
    const auto store = makeStore(backend, root.path());
    QVERIFY2(store->open(), qPrintable(store->lastError()));

    QVERIFY(store->saveMetrics(QStringLiteral("all-time"), recordedSnapshot()));
    QVERIFY(store->saveMetrics(QStringLiteral("session"), MetricsSnapshot()));
    QCOMPARE(store->metricsIds(),
             QStringList({QStringLiteral("all-time"), QStringLiteral("session")}));

    QVERIFY(store->removeMetrics(QStringLiteral("session")));
    QCOMPARE(store->metricsIds(), QStringList({QStringLiteral("all-time")}));
    QVERIFY(!store->loadMetrics(QStringLiteral("session")).has_value());
}

void TestStore::refusesAStoreWrittenByANewerVersion()
{
    // Reading it on a guess is how the newer version's data gets lost. Failing
    // to open says so to a caller that can still tell the user.
    QFETCH(QString, backend);
    QTemporaryDir root;
    stampSchemaVersion(backend, root.path(), Store::CurrentSchemaVersion + 1);

    const auto store = makeStore(backend, root.path());
    QVERIFY(!store->open());
    QVERIFY(!store->isOpen());
    QVERIFY(store->lastError().contains(QStringLiteral("newer")));
}

void TestStore::aClosedStoreFailsRatherThanPretending()
{
    QFETCH(QString, backend);
    QTemporaryDir root;
    const auto store = makeStore(backend, root.path());

    Transcript transcript;
    transcript.addUserMessage(QStringLiteral("hello"));
    QVERIFY(!store->saveConversation(QStringLiteral("c"), transcript));
    QVERIFY(!store->lastError().isEmpty());
    QVERIFY(!store->loadConversation(QStringLiteral("c")).has_value());
    QCOMPARE(store->schemaVersion(), 0);

    QVERIFY2(store->open(), qPrintable(store->lastError()));
    QVERIFY(store->saveConversation(QStringLiteral("c"), transcript));
    // The error is the last one, not the worst one ever seen.
    QVERIFY(store->lastError().isEmpty());

    store->close();
    QVERIFY(!store->isOpen());
    QVERIFY(!store->saveConversation(QStringLiteral("c"), transcript));

    // A bounded listing and a batch refuse for the same reason rather than
    // reporting an empty store and a batch that nothing started.
    QVERIFY(store->conversations(10).isEmpty());
    QVERIFY(!store->beginBatch());
    QVERIFY(!store->lastError().isEmpty());
    const Store::Batch batch(store.get());
    QVERIFY(!batch.isActive());
}

QTEST_MAIN(TestStore)
#include "tst_store.moc"
