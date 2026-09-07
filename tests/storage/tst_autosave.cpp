// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/MetricsCollector.h>
#include <QtOpenAi/Storage/Autosave.h>
#include <QtOpenAi/Storage/JsonFileStore.h>

#include <QtCore/QTemporaryDir>
#include <QtTest/QtTest>

#include "support/BatchCountingStore.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Chat;
using namespace QtOpenAi::Client;
using namespace QtOpenAi::Storage;

// Coverage for the autosave hook (#48).
class TestAutosave : public QObject
{
    Q_OBJECT
private slots:
    void manyChangesInOneIntervalAreOneSave();
    void flushWritesTheCurrentConversation();
    void aCollectorMarksItselfDirty();
    void disablingSuspendsTheWriteButNotTheChange();
    void oneFlushIsOneBatch();
    void aFailedSaveIsReportedAndStaysDirty();
};

void TestAutosave::manyChangesInOneIntervalAreOneSave()
{
    // The reason the class exists: a save per change is a file write per
    // streamed fragment.
    QTemporaryDir root;
    JsonFileStore store(root.path());
    QVERIFY2(store.open(), qPrintable(store.lastError()));

    Transcript transcript;
    Autosave autosave(&store);
    autosave.setIntervalMs(50);
    autosave.setConversation(QStringLiteral("c"), [&transcript] { return transcript; });
    QSignalSpy saves(&autosave, &Autosave::saved);

    for (int i = 0; i < 5; ++i) {
        transcript.addUserMessage(QStringLiteral("message %1").arg(i));
        autosave.touch();
    }
    QVERIFY(autosave.isDirty());
    QCOMPARE(saves.count(), 0); // nothing yet: the interval has not elapsed

    QVERIFY(saves.wait(2000));
    QCOMPARE(saves.count(), 1);
    QVERIFY(!autosave.isDirty());

    // What landed is the state at save time, not the state at the first touch.
    const std::optional<Transcript> saved = store.loadConversation(QStringLiteral("c"));
    QVERIFY(saved.has_value());
    QCOMPARE(saved->count(), 5);
}

void TestAutosave::flushWritesTheCurrentConversation()
{
    QTemporaryDir root;
    JsonFileStore store(root.path());
    QVERIFY2(store.open(), qPrintable(store.lastError()));

    Transcript transcript;
    transcript.addUserMessage(QStringLiteral("hello"));

    Autosave autosave(&store);
    autosave.setIntervalMs(60000); // far away: only flush() can save here
    autosave.setConversation(QStringLiteral("c"), [&transcript] { return transcript; });
    QCOMPARE(autosave.conversationId(), QStringLiteral("c"));

    autosave.touch();
    QVERIFY(autosave.flush());
    QVERIFY(!autosave.isDirty());

    const std::optional<Transcript> saved = store.loadConversation(QStringLiteral("c"));
    QVERIFY(saved.has_value());
    QCOMPARE(*saved, transcript);

    // A flush with nothing to write has done what was asked.
    QVERIFY(autosave.flush());
}

void TestAutosave::aCollectorMarksItselfDirty()
{
    // Metrics need no touch(): the collector announces every request, which is
    // the only event that can change the snapshot.
    QTemporaryDir root;
    JsonFileStore store(root.path());
    QVERIFY2(store.open(), qPrintable(store.lastError()));

    MetricsCollector collector;
    Autosave autosave(&store);
    autosave.setIntervalMs(0); // save on the spot, so the test has no timer in it
    autosave.setMetrics(QStringLiteral("all-time"), &collector);
    QCOMPARE(autosave.metricsId(), QStringLiteral("all-time"));

    RequestMetrics request;
    request.model = QStringLiteral("gpt-4o-mini");
    request.durationMs = 120;
    request.ok = true;
    collector.recordRequest(request);

    const std::optional<MetricsSnapshot> saved = store.loadMetrics(QStringLiteral("all-time"));
    QVERIFY(saved.has_value());
    QCOMPARE(saved->requests, 1);
    QCOMPARE(saved->successes, 1);
    QCOMPARE(saved->totalDurationMs, qint64(120));

    // And the collector picks it back up, which is what makes the totals the
    // user's rather than this process's.
    MetricsCollector restored;
    restored.restore(*saved);
    QCOMPARE(restored.snapshot().requests, 1);
    QCOMPARE(restored.metrics(QStringLiteral("gpt-4o-mini")).requests, 1);
}

void TestAutosave::disablingSuspendsTheWriteButNotTheChange()
{
    QTemporaryDir root;
    JsonFileStore store(root.path());
    QVERIFY2(store.open(), qPrintable(store.lastError()));

    Transcript transcript;
    transcript.addUserMessage(QStringLiteral("hello"));

    Autosave autosave(&store);
    autosave.setIntervalMs(10);
    autosave.setConversation(QStringLiteral("c"), [&transcript] { return transcript; });
    autosave.setEnabled(false);

    autosave.touch();
    QTest::qWait(60);
    QVERIFY(autosave.isDirty());
    QVERIFY(!store.loadConversation(QStringLiteral("c")).has_value());

    // Enabling again saves what happened while it was off rather than losing
    // it: the change was recorded, only the writing was suspended.
    QSignalSpy saves(&autosave, &Autosave::saved);
    autosave.setEnabled(true);
    QVERIFY(saves.wait(2000));
    QVERIFY(store.loadConversation(QStringLiteral("c")).has_value());
}

void TestAutosave::oneFlushIsOneBatch()
{
    // The conversation and the metrics snapshot are the two writes of every
    // interval, and grouping them is what makes an interval one commit on a
    // backend that has transactions rather than two.
    QTemporaryDir root;
    BatchCountingStore store(root.path());
    QVERIFY2(store.open(), qPrintable(store.lastError()));

    Transcript transcript;
    transcript.addUserMessage(QStringLiteral("hello"));
    MetricsCollector collector;

    Autosave autosave(&store);
    autosave.setIntervalMs(60000); // far away: only flush() can save here
    autosave.setConversation(QStringLiteral("c"), [&transcript] { return transcript; });
    autosave.setMetrics(QStringLiteral("all-time"), &collector);

    autosave.touch();
    QVERIFY(autosave.flush());
    QCOMPARE(store.batchesBegun, 1);
    QCOMPARE(store.batchesEnded, 1);
    QCOMPARE(store.batchesDropped, 0);
    // Both writes are inside that one batch.
    QVERIFY(store.loadConversation(QStringLiteral("c")).has_value());
    QVERIFY(store.loadMetrics(QStringLiteral("all-time")).has_value());

    // A flush with nothing to write opens no batch either.
    QVERIFY(autosave.flush());
    QCOMPARE(store.batchesBegun, 1);
}

void TestAutosave::aFailedSaveIsReportedAndStaysDirty()
{
    // A silent autosave failure is data loss nobody hears about.
    QTemporaryDir root;
    BatchCountingStore store(root.path()); // deliberately not opened

    Transcript transcript;
    transcript.addUserMessage(QStringLiteral("hello"));

    Autosave autosave(&store);
    autosave.setIntervalMs(60000);
    autosave.setConversation(QStringLiteral("c"), [&transcript] { return transcript; });
    QSignalSpy failures(&autosave, &Autosave::failed);

    autosave.touch();
    QVERIFY(!autosave.flush());
    QCOMPARE(failures.count(), 1);
    QVERIFY(!failures.at(0).at(0).toString().isEmpty());
    // Still dirty: what failed is not on disk, and the next attempt must try
    // again rather than assume it is safe.
    QVERIFY(autosave.isDirty());
    // One failure, not two: the store never started a batch, so there is none
    // to end and nothing to report a second time.
    QCOMPARE(store.batchesBegun, 1);
    QCOMPARE(store.batchesEnded, 0);
}

QTEST_MAIN(TestAutosave)
#include "tst_autosave.moc"
