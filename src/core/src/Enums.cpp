// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Enums.h"

namespace QtOpenAi {
namespace Core {

namespace {

// The wire spelling of one enum value. Each enum below is described by a single
// table, so the mapping and its inverse can never drift apart -- which they
// could when both directions were hand-written as a switch and an if-chain.
template <typename Enum>
struct WireName
{
    Enum value;
    const char *name;
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
        {VideoStatus::Completed, "completed"},
        {VideoStatus::Failed, "failed"},
};

constexpr WireName<UploadStatus> kUploadStatuses[] = {
        {UploadStatus::Pending, "pending"},
        {UploadStatus::Completed, "completed"},
        {UploadStatus::Cancelled, "cancelled"},
        {UploadStatus::Expired, "expired"},
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

constexpr WireName<BatchStatus> kBatchStatuses[] = {
        {BatchStatus::Validating, "validating"}, {BatchStatus::InProgress, "in_progress"},
        {BatchStatus::Finalizing, "finalizing"}, {BatchStatus::Completed, "completed"},
        {BatchStatus::Failed, "failed"},         {BatchStatus::Expired, "expired"},
        {BatchStatus::Cancelling, "cancelling"}, {BatchStatus::Cancelled, "cancelled"},
};

constexpr WireName<FineTuningJobStatus> kFineTuningJobStatuses[] = {
        {FineTuningJobStatus::ValidatingFiles, "validating_files"},
        {FineTuningJobStatus::Queued, "queued"},
        {FineTuningJobStatus::Running, "running"},
        {FineTuningJobStatus::Succeeded, "succeeded"},
        {FineTuningJobStatus::Failed, "failed"},
        {FineTuningJobStatus::Cancelled, "cancelled"},
        {FineTuningJobStatus::Paused, "paused"},
};

// Note the single-l "canceled" this endpoint family uses, unlike every other.
constexpr WireName<EvalRunStatus> kEvalRunStatuses[] = {
        {EvalRunStatus::Queued, "queued"},       {EvalRunStatus::InProgress, "in_progress"},
        {EvalRunStatus::Completed, "completed"}, {EvalRunStatus::Failed, "failed"},
        {EvalRunStatus::Canceled, "canceled"},
};

} // namespace

QString roleToString(Role role) { return toWire(role, kRoles, Role::User); }
Role roleFromString(const QString &value) { return fromWire(value, kRoles, Role::User); }

QString finishReasonToString(FinishReason reason)
{
    return toWire(reason, kFinishReasons, FinishReason::None);
}

FinishReason finishReasonFromString(const QString &value)
{
    return fromWire(value, kFinishReasons, FinishReason::None);
}

QString videoStatusToString(VideoStatus status)
{
    return toWire(status, kVideoStatuses, VideoStatus::Queued);
}

VideoStatus videoStatusFromString(const QString &value)
{
    return fromWire(value, kVideoStatuses, VideoStatus::Queued);
}

QString uploadStatusToString(UploadStatus status)
{
    return toWire(status, kUploadStatuses, UploadStatus::Pending);
}

UploadStatus uploadStatusFromString(const QString &value)
{
    return fromWire(value, kUploadStatuses, UploadStatus::Pending);
}

QString vectorStoreStatusToString(VectorStoreStatus status)
{
    return toWire(status, kVectorStoreStatuses, VectorStoreStatus::InProgress);
}

VectorStoreStatus vectorStoreStatusFromString(const QString &value)
{
    return fromWire(value, kVectorStoreStatuses, VectorStoreStatus::InProgress);
}

QString vectorStoreFileStatusToString(VectorStoreFileStatus status)
{
    return toWire(status, kVectorStoreFileStatuses, VectorStoreFileStatus::InProgress);
}

VectorStoreFileStatus vectorStoreFileStatusFromString(const QString &value)
{
    return fromWire(value, kVectorStoreFileStatuses, VectorStoreFileStatus::InProgress);
}

QString batchStatusToString(BatchStatus status)
{
    return toWire(status, kBatchStatuses, BatchStatus::Validating);
}

BatchStatus batchStatusFromString(const QString &value)
{
    return fromWire(value, kBatchStatuses, BatchStatus::Validating);
}

QString fineTuningJobStatusToString(FineTuningJobStatus status)
{
    return toWire(status, kFineTuningJobStatuses, FineTuningJobStatus::Queued);
}

FineTuningJobStatus fineTuningJobStatusFromString(const QString &value)
{
    return fromWire(value, kFineTuningJobStatuses, FineTuningJobStatus::Queued);
}

QString evalRunStatusToString(EvalRunStatus status)
{
    return toWire(status, kEvalRunStatuses, EvalRunStatus::Queued);
}

EvalRunStatus evalRunStatusFromString(const QString &value)
{
    return fromWire(value, kEvalRunStatuses, EvalRunStatus::Queued);
}

} // namespace Core
} // namespace QtOpenAi
