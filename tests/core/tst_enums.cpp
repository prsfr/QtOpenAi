// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/Enums.h>

#include <QtCore/QMetaEnum>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// The enum wire conversions are table-driven, so nothing makes the compiler
// complain when an enum gains a value and its table does not. These tests close
// that gap from the other side: they ask the meta-object system for every value
// each enum actually declares and require it to survive a round trip. A missing
// table row collapses the value onto the fallback and fails here — and unlike a
// switch's exhaustiveness warning, this also catches a row that exists but
// carries the wrong spelling.
class TestEnums : public QObject
{
    Q_OBJECT
private slots:
    void roleRoundTripsEveryValue();
    void finishReasonRoundTripsEveryValue();
    void videoStatusRoundTripsEveryValue();
    void uploadStatusRoundTripsEveryValue();
    void vectorStoreStatusRoundTripsEveryValue();
    void vectorStoreFileStatusRoundTripsEveryValue();
    void batchStatusRoundTripsEveryValue();
    void fineTuningJobStatusRoundTripsEveryValue();
    void evalRunStatusRoundTripsEveryValue();
    void runStatusRoundTripsEveryValue();
    void runStepStatusRoundTripsEveryValue();
    void chatKitSessionStatusRoundTripsEveryValue();
    void chatKitThreadStatusRoundTripsEveryValue();
    void terminalStatesAreExactlyTheDocumentedOnes();
    void unknownStatusIsNeverTerminal();
    void unknownWireValueDecodesToTheDocumentedFallback();
    void spellingsAreDistinct();
};

namespace {

// Every value the meta-object system reports for `Enum`.
template <typename Enum>
QList<Enum> declaredValues()
{
    const QMetaEnum meta = QMetaEnum::fromType<Enum>();
    QList<Enum> values;
    for (int i = 0; i < meta.keyCount(); ++i)
        values.append(static_cast<Enum>(meta.value(i)));
    return values;
}

// Round-trip every declared value, and require the spellings to be unique so
// two values cannot silently share a row.
template <typename Enum, typename ToString, typename FromString>
void checkRoundTrip(ToString toString, FromString fromString)
{
    const QList<Enum> values = declaredValues<Enum>();
    QVERIFY2(!values.isEmpty(), "enum is not registered with Q_ENUM_NS");

    QSet<QString> seen;
    for (Enum value : values) {
        const QString wire = toString(value);
        const Enum back = fromString(wire);
        QVERIFY2(back == value,
                 qPrintable(QStringLiteral("value %1 encodes as \"%2\" but decodes back to %3")
                                    .arg(int(value))
                                    .arg(wire)
                                    .arg(int(back))));
        QVERIFY2(!seen.contains(wire),
                 qPrintable(QStringLiteral("spelling \"%1\" is used twice").arg(wire)));
        seen.insert(wire);
    }
}

} // namespace

void TestEnums::roleRoundTripsEveryValue() { checkRoundTrip<Role>(roleToString, roleFromString); }

void TestEnums::finishReasonRoundTripsEveryValue()
{
    // Includes FinishReason::None, whose spelling is deliberately the empty
    // string because the field is absent rather than set.
    checkRoundTrip<FinishReason>(finishReasonToString, finishReasonFromString);
}

void TestEnums::videoStatusRoundTripsEveryValue()
{
    checkRoundTrip<VideoStatus>(videoStatusToString, videoStatusFromString);
}

void TestEnums::uploadStatusRoundTripsEveryValue()
{
    checkRoundTrip<UploadStatus>(uploadStatusToString, uploadStatusFromString);
}

void TestEnums::vectorStoreStatusRoundTripsEveryValue()
{
    checkRoundTrip<VectorStoreStatus>(vectorStoreStatusToString, vectorStoreStatusFromString);
}

void TestEnums::vectorStoreFileStatusRoundTripsEveryValue()
{
    checkRoundTrip<VectorStoreFileStatus>(vectorStoreFileStatusToString,
                                          vectorStoreFileStatusFromString);
}

void TestEnums::batchStatusRoundTripsEveryValue()
{
    checkRoundTrip<BatchStatus>(batchStatusToString, batchStatusFromString);
}

void TestEnums::fineTuningJobStatusRoundTripsEveryValue()
{
    checkRoundTrip<FineTuningJobStatus>(fineTuningJobStatusToString, fineTuningJobStatusFromString);
}

void TestEnums::evalRunStatusRoundTripsEveryValue()
{
    checkRoundTrip<EvalRunStatus>(evalRunStatusToString, evalRunStatusFromString);
}

void TestEnums::runStatusRoundTripsEveryValue()
{
    checkRoundTrip<RunStatus>(runStatusToString, runStatusFromString);
}

void TestEnums::runStepStatusRoundTripsEveryValue()
{
    checkRoundTrip<RunStepStatus>(runStepStatusToString, runStepStatusFromString);
}

void TestEnums::chatKitSessionStatusRoundTripsEveryValue()
{
    checkRoundTrip<ChatKitSessionStatus>(chatKitSessionStatusToString,
                                         chatKitSessionStatusFromString);
}

void TestEnums::chatKitThreadStatusRoundTripsEveryValue()
{
    checkRoundTrip<ChatKitThreadStatus>(chatKitThreadStatusToString, chatKitThreadStatusFromString);
}

void TestEnums::terminalStatesAreExactlyTheDocumentedOnes()
{
    // Terminality now lives in the same table rows as the wire spelling, which
    // is what stops the two from disagreeing -- but a wrong column would be
    // silent. This pins the whole set per enum, so flipping any one row fails
    // here rather than in a poller that stops early or never stops.
    QCOMPARE(isTerminal(VideoStatus::Completed), true);
    QCOMPARE(isTerminal(VideoStatus::Failed), true);
    QCOMPARE(isTerminal(VideoStatus::Queued), false);
    QCOMPARE(isTerminal(VideoStatus::InProgress), false);

    // Only a pending upload accepts further parts.
    QCOMPARE(isTerminal(UploadStatus::Pending), false);
    QCOMPARE(isTerminal(UploadStatus::Completed), true);
    QCOMPARE(isTerminal(UploadStatus::Cancelled), true);
    QCOMPARE(isTerminal(UploadStatus::Expired), true);

    // Cancelling still has in-flight requests to drain.
    QCOMPARE(isTerminal(BatchStatus::Cancelling), false);
    QCOMPARE(isTerminal(BatchStatus::Cancelled), true);
    QCOMPARE(isTerminal(BatchStatus::Completed), true);
    QCOMPARE(isTerminal(BatchStatus::Failed), true);
    QCOMPARE(isTerminal(BatchStatus::Expired), true);
    QCOMPARE(isTerminal(BatchStatus::Validating), false);
    QCOMPARE(isTerminal(BatchStatus::InProgress), false);
    QCOMPARE(isTerminal(BatchStatus::Finalizing), false);

    // A paused job resumes on request, so a poller keeps waiting.
    QCOMPARE(isTerminal(FineTuningJobStatus::Paused), false);
    QCOMPARE(isTerminal(FineTuningJobStatus::Succeeded), true);
    QCOMPARE(isTerminal(FineTuningJobStatus::Failed), true);
    QCOMPARE(isTerminal(FineTuningJobStatus::Cancelled), true);
    QCOMPARE(isTerminal(FineTuningJobStatus::ValidatingFiles), false);
    QCOMPARE(isTerminal(FineTuningJobStatus::Queued), false);
    QCOMPARE(isTerminal(FineTuningJobStatus::Running), false);

    QCOMPARE(isTerminal(EvalRunStatus::Completed), true);
    QCOMPARE(isTerminal(EvalRunStatus::Failed), true);
    QCOMPARE(isTerminal(EvalRunStatus::Canceled), true);
    QCOMPARE(isTerminal(EvalRunStatus::Queued), false);
    QCOMPARE(isTerminal(EvalRunStatus::InProgress), false);

    // RequiresAction is parked, not finished: the run continues once the tool
    // outputs arrive.
    QCOMPARE(isTerminal(RunStatus::RequiresAction), false);
    QCOMPARE(isTerminal(RunStatus::Cancelling), false);
    QCOMPARE(isTerminal(RunStatus::Queued), false);
    QCOMPARE(isTerminal(RunStatus::InProgress), false);
    QCOMPARE(isTerminal(RunStatus::Completed), true);
    QCOMPARE(isTerminal(RunStatus::Failed), true);
    QCOMPARE(isTerminal(RunStatus::Cancelled), true);
    QCOMPARE(isTerminal(RunStatus::Incomplete), true);
    QCOMPARE(isTerminal(RunStatus::Expired), true);

    QCOMPARE(isTerminal(RunStepStatus::InProgress), false);
    QCOMPARE(isTerminal(RunStepStatus::Cancelled), true);
    QCOMPARE(isTerminal(RunStepStatus::Failed), true);
    QCOMPARE(isTerminal(RunStepStatus::Completed), true);
    QCOMPARE(isTerminal(RunStepStatus::Expired), true);
}

void TestEnums::unknownStatusIsNeverTerminal()
{
    // A status from a newer server decodes to the non-terminal fallback, and
    // the terminality lookup agrees: a poller keeps waiting rather than
    // reporting a job finished that is not.
    const QString unknown = QStringLiteral("something_the_server_invented");
    QVERIFY(!isTerminal(batchStatusFromString(unknown)));
    QVERIFY(!isTerminal(runStatusFromString(unknown)));
    QVERIFY(!isTerminal(fineTuningJobStatusFromString(unknown)));
    QVERIFY(!isTerminal(evalRunStatusFromString(unknown)));
    QVERIFY(!isTerminal(videoStatusFromString(unknown)));
    QVERIFY(!isTerminal(uploadStatusFromString(unknown)));
    QVERIFY(!isTerminal(runStepStatusFromString(unknown)));
}

void TestEnums::unknownWireValueDecodesToTheDocumentedFallback()
{
    // Each fallback is the state a client should keep waiting in, so an
    // unfamiliar status from a newer server never looks terminal.
    const QString unknown = QStringLiteral("something_the_server_invented");
    QCOMPARE(roleFromString(unknown), Role::User);
    QCOMPARE(finishReasonFromString(unknown), FinishReason::None);
    QCOMPARE(videoStatusFromString(unknown), VideoStatus::Queued);
    QCOMPARE(uploadStatusFromString(unknown), UploadStatus::Pending);
    QCOMPARE(vectorStoreStatusFromString(unknown), VectorStoreStatus::InProgress);
    QCOMPARE(vectorStoreFileStatusFromString(unknown), VectorStoreFileStatus::InProgress);
    QCOMPARE(batchStatusFromString(unknown), BatchStatus::Validating);
    QCOMPARE(fineTuningJobStatusFromString(unknown), FineTuningJobStatus::Queued);
    QCOMPARE(evalRunStatusFromString(unknown), EvalRunStatus::Queued);
    QCOMPARE(runStatusFromString(unknown), RunStatus::Queued);
    QCOMPARE(runStepStatusFromString(unknown), RunStepStatus::InProgress);
    // Neither ChatKit status is polled, so their fallback is the state that
    // keeps a UI usable rather than one that keeps a poller waiting.
    QCOMPARE(chatKitSessionStatusFromString(unknown), ChatKitSessionStatus::Active);
    QCOMPARE(chatKitThreadStatusFromString(unknown), ChatKitThreadStatus::Active);
}

void TestEnums::spellingsAreDistinct()
{
    // The one spelling that differs from every sibling: Evals says "canceled"
    // with a single l, while the rest of the API says "cancelled".
    QCOMPARE(evalRunStatusToString(EvalRunStatus::Canceled), QStringLiteral("canceled"));
    QCOMPARE(uploadStatusToString(UploadStatus::Cancelled), QStringLiteral("cancelled"));
    QCOMPARE(batchStatusToString(BatchStatus::Cancelled), QStringLiteral("cancelled"));
    QCOMPARE(runStatusToString(RunStatus::Cancelled), QStringLiteral("cancelled"));
    // ...and a "cancelled" spelling must not decode as an eval's Canceled.
    QCOMPARE(evalRunStatusFromString(QStringLiteral("cancelled")), EvalRunStatus::Queued);
}

QTEST_MAIN(TestEnums)
#include "tst_enums.moc"
