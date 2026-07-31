// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Enums.h"

namespace QtOpenAi {
namespace Core {

namespace {

// One enum value: its wire spelling, and whether reaching it ends the lifecycle
// the value belongs to. Each enum below is described by a single table, so the
// mapping and its inverse can never drift apart -- which they could when both
// directions were hand-written as a switch and an if-chain.
//
// `terminal` extends that same argument to the third thing a status is asked:
// the job-shaped enums used to answer isTerminal() from a switch inside their
// value type, one file away from the spelling. A status added here with the
// wrong column, or added to the type's switch and not here, could disagree with
// itself. Now there is one row per value and nowhere else to look. The enums
// that describe no lifecycle (Role, FinishReason) leave the column at false and
// are never asked.
template <typename Enum>
struct WireName
{
    Enum value;
    const char *name;
    bool terminal = false;
};

// An enum value the table does not cover encodes as `fallback`'s spelling, and
// an unrecognised wire string decodes to `fallback`. Every table is proven
// complete by tst_core_enums, which round-trips every value the meta-object
// system reports -- a stronger check than a switch's exhaustiveness warning,
// because it also catches a value that has a row but the wrong spelling.
template <typename Enum, size_t N>
QString toWire(Enum value, const WireName<Enum> (&table)[N], Enum fallback)
{
    for (const auto &row : table) {
        if (row.value == value)
            return QString::fromLatin1(row.name);
    }
    return toWire(fallback, table, fallback);
}

template <typename Enum, size_t N>
Enum fromWire(const QString &value, const WireName<Enum> (&table)[N], Enum fallback)
{
    for (const auto &row : table) {
        if (value == QLatin1String(row.name))
            return row.value;
    }
    return fallback;
}

// A value the table does not cover is treated as non-terminal, matching the
// decode fallbacks: an unfamiliar status from a newer server leaves a poller
// waiting rather than stopping it early.
template <typename Enum, size_t N>
bool terminalIn(Enum value, const WireName<Enum> (&table)[N])
{
    for (const auto &row : table) {
        if (row.value == value)
            return row.terminal;
    }
    return false;
}

constexpr WireName<Role> kRoles[] = {
        {Role::System, "system"}, {Role::User, "user"},           {Role::Assistant, "assistant"},
        {Role::Tool, "tool"},     {Role::Developer, "developer"},
};

// FinishReason::None is the absent field, so its spelling is the empty string.
constexpr WireName<FinishReason> kFinishReasons[] = {
        {FinishReason::None, ""},
        {FinishReason::Stop, "stop"},
        {FinishReason::Length, "length"},
        {FinishReason::ToolCalls, "tool_calls"},
        {FinishReason::ContentFilter, "content_filter"},
        {FinishReason::FunctionCall, "function_call"},
};

constexpr WireName<VideoStatus> kVideoStatuses[] = {
        {VideoStatus::Queued, "queued"},
        {VideoStatus::InProgress, "in_progress"},
        {VideoStatus::Completed, "completed", true},
        {VideoStatus::Failed, "failed", true},
};

// Only a pending upload accepts further parts, so every other state is final.
constexpr WireName<UploadStatus> kUploadStatuses[] = {
        {UploadStatus::Pending, "pending"},
        {UploadStatus::Completed, "completed", true},
        {UploadStatus::Cancelled, "cancelled", true},
        {UploadStatus::Expired, "expired", true},
};

constexpr WireName<VectorStoreStatus> kVectorStoreStatuses[] = {
        {VectorStoreStatus::InProgress, "in_progress"},
        {VectorStoreStatus::Completed, "completed"},
        {VectorStoreStatus::Expired, "expired"},
};

constexpr WireName<VectorStoreFileStatus> kVectorStoreFileStatuses[] = {
        {VectorStoreFileStatus::InProgress, "in_progress"},
        {VectorStoreFileStatus::Completed, "completed"},
        {VectorStoreFileStatus::Cancelled, "cancelled"},
        {VectorStoreFileStatus::Failed, "failed"},
};

// Cancelling is not terminal: the batch still has to drain its in-flight
// requests before it reaches Cancelled.
constexpr WireName<BatchStatus> kBatchStatuses[] = {
        {BatchStatus::Validating, "validating"}, {BatchStatus::InProgress, "in_progress"},
        {BatchStatus::Finalizing, "finalizing"}, {BatchStatus::Completed, "completed", true},
        {BatchStatus::Failed, "failed", true},   {BatchStatus::Expired, "expired", true},
        {BatchStatus::Cancelling, "cancelling"}, {BatchStatus::Cancelled, "cancelled", true},
};

// A paused job is not terminal -- it resumes on request, so a poller keeps
// waiting across the pause.
constexpr WireName<FineTuningJobStatus> kFineTuningJobStatuses[] = {
        {FineTuningJobStatus::ValidatingFiles, "validating_files"},
        {FineTuningJobStatus::Queued, "queued"},
        {FineTuningJobStatus::Running, "running"},
        {FineTuningJobStatus::Succeeded, "succeeded", true},
        {FineTuningJobStatus::Failed, "failed", true},
        {FineTuningJobStatus::Cancelled, "cancelled", true},
        {FineTuningJobStatus::Paused, "paused"},
};

// Note the single-l "canceled" this endpoint family uses, unlike every other.
constexpr WireName<EvalRunStatus> kEvalRunStatuses[] = {
        {EvalRunStatus::Queued, "queued"},
        {EvalRunStatus::InProgress, "in_progress"},
        {EvalRunStatus::Completed, "completed", true},
        {EvalRunStatus::Failed, "failed", true},
        {EvalRunStatus::Canceled, "canceled", true},
};

// RequiresAction is deliberately not terminal: the run is parked until the
// client submits its tool outputs, and then continues.
constexpr WireName<RunStatus> kRunStatuses[] = {
        {RunStatus::Queued, "queued"},
        {RunStatus::InProgress, "in_progress"},
        {RunStatus::RequiresAction, "requires_action"},
        {RunStatus::Cancelling, "cancelling"},
        {RunStatus::Cancelled, "cancelled", true},
        {RunStatus::Failed, "failed", true},
        {RunStatus::Completed, "completed", true},
        {RunStatus::Incomplete, "incomplete", true},
        {RunStatus::Expired, "expired", true},
};

constexpr WireName<RunStepStatus> kRunStepStatuses[] = {
        {RunStepStatus::InProgress, "in_progress"}, {RunStepStatus::Cancelled, "cancelled", true},
        {RunStepStatus::Failed, "failed", true},    {RunStepStatus::Completed, "completed", true},
        {RunStepStatus::Expired, "expired", true},
};

// Nothing polls a ChatKit session or thread, so neither table claims a terminal
// state — the column stays at its default, like Role and FinishReason.
constexpr WireName<ChatKitSessionStatus> kChatKitSessionStatuses[] = {
        {ChatKitSessionStatus::Active, "active"},
        {ChatKitSessionStatus::Expired, "expired"},
        {ChatKitSessionStatus::Cancelled, "cancelled"},
};

constexpr WireName<ChatKitThreadStatus> kChatKitThreadStatuses[] = {
        {ChatKitThreadStatus::Active, "active"},
        {ChatKitThreadStatus::Locked, "locked"},
        {ChatKitThreadStatus::Closed, "closed"},
};

} // namespace

// Define one enum's pair of public conversions. Both directions name the same
// table and the same fallback exactly once here, so a fallback cannot be
// changed on the decode side and forgotten on the encode side -- the drift the
// tables themselves were introduced to remove, one level up.
#define QTOPENAI_WIRE_CONVERSIONS(ToName, FromName, Enum, table, fallback)                         \
    QString ToName(Enum value) { return toWire(value, table, fallback); }                          \
    Enum FromName(const QString &value) { return fromWire(value, table, fallback); }

QTOPENAI_WIRE_CONVERSIONS(roleToString, roleFromString, Role, kRoles, Role::User)
QTOPENAI_WIRE_CONVERSIONS(finishReasonToString, finishReasonFromString, FinishReason,
                          kFinishReasons, FinishReason::None)
QTOPENAI_WIRE_CONVERSIONS(videoStatusToString, videoStatusFromString, VideoStatus, kVideoStatuses,
                          VideoStatus::Queued)
QTOPENAI_WIRE_CONVERSIONS(uploadStatusToString, uploadStatusFromString, UploadStatus,
                          kUploadStatuses, UploadStatus::Pending)
QTOPENAI_WIRE_CONVERSIONS(vectorStoreStatusToString, vectorStoreStatusFromString, VectorStoreStatus,
                          kVectorStoreStatuses, VectorStoreStatus::InProgress)
QTOPENAI_WIRE_CONVERSIONS(vectorStoreFileStatusToString, vectorStoreFileStatusFromString,
                          VectorStoreFileStatus, kVectorStoreFileStatuses,
                          VectorStoreFileStatus::InProgress)
QTOPENAI_WIRE_CONVERSIONS(batchStatusToString, batchStatusFromString, BatchStatus, kBatchStatuses,
                          BatchStatus::Validating)
QTOPENAI_WIRE_CONVERSIONS(fineTuningJobStatusToString, fineTuningJobStatusFromString,
                          FineTuningJobStatus, kFineTuningJobStatuses, FineTuningJobStatus::Queued)
QTOPENAI_WIRE_CONVERSIONS(evalRunStatusToString, evalRunStatusFromString, EvalRunStatus,
                          kEvalRunStatuses, EvalRunStatus::Queued)
QTOPENAI_WIRE_CONVERSIONS(runStatusToString, runStatusFromString, RunStatus, kRunStatuses,
                          RunStatus::Queued)
QTOPENAI_WIRE_CONVERSIONS(runStepStatusToString, runStepStatusFromString, RunStepStatus,
                          kRunStepStatuses, RunStepStatus::InProgress)
QTOPENAI_WIRE_CONVERSIONS(chatKitSessionStatusToString, chatKitSessionStatusFromString,
                          ChatKitSessionStatus, kChatKitSessionStatuses,
                          ChatKitSessionStatus::Active)
QTOPENAI_WIRE_CONVERSIONS(chatKitThreadStatusToString, chatKitThreadStatusFromString,
                          ChatKitThreadStatus, kChatKitThreadStatuses, ChatKitThreadStatus::Active)

#undef QTOPENAI_WIRE_CONVERSIONS

// The lifecycle question, answered from the same rows as the spelling.
bool isTerminal(VideoStatus status) { return terminalIn(status, kVideoStatuses); }
bool isTerminal(UploadStatus status) { return terminalIn(status, kUploadStatuses); }
bool isTerminal(BatchStatus status) { return terminalIn(status, kBatchStatuses); }
bool isTerminal(FineTuningJobStatus status) { return terminalIn(status, kFineTuningJobStatuses); }
bool isTerminal(EvalRunStatus status) { return terminalIn(status, kEvalRunStatuses); }
bool isTerminal(RunStatus status) { return terminalIn(status, kRunStatuses); }
bool isTerminal(RunStepStatus status) { return terminalIn(status, kRunStepStatuses); }

} // namespace Core
} // namespace QtOpenAi
