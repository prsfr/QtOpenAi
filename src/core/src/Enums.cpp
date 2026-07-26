// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Enums.h"

namespace QtOpenAi {
namespace Core {

QString roleToString(Role role)
{
    switch (role) {
    case Role::System:
        return QStringLiteral("system");
    case Role::User:
        return QStringLiteral("user");
    case Role::Assistant:
        return QStringLiteral("assistant");
    case Role::Tool:
        return QStringLiteral("tool");
    case Role::Developer:
        return QStringLiteral("developer");
    }
    return QStringLiteral("user");
}

Role roleFromString(const QString &value)
{
    if (value == QLatin1String("system"))
        return Role::System;
    if (value == QLatin1String("assistant"))
        return Role::Assistant;
    if (value == QLatin1String("tool"))
        return Role::Tool;
    if (value == QLatin1String("developer"))
        return Role::Developer;
    return Role::User;
}

QString finishReasonToString(FinishReason reason)
{
    switch (reason) {
    case FinishReason::None:
        return QString();
    case FinishReason::Stop:
        return QStringLiteral("stop");
    case FinishReason::Length:
        return QStringLiteral("length");
    case FinishReason::ToolCalls:
        return QStringLiteral("tool_calls");
    case FinishReason::ContentFilter:
        return QStringLiteral("content_filter");
    case FinishReason::FunctionCall:
        return QStringLiteral("function_call");
    }
    return QString();
}

FinishReason finishReasonFromString(const QString &value)
{
    if (value == QLatin1String("stop"))
        return FinishReason::Stop;
    if (value == QLatin1String("length"))
        return FinishReason::Length;
    if (value == QLatin1String("tool_calls"))
        return FinishReason::ToolCalls;
    if (value == QLatin1String("content_filter"))
        return FinishReason::ContentFilter;
    if (value == QLatin1String("function_call"))
        return FinishReason::FunctionCall;
    return FinishReason::None;
}

QString videoStatusToString(VideoStatus status)
{
    switch (status) {
    case VideoStatus::Queued:
        return QStringLiteral("queued");
    case VideoStatus::InProgress:
        return QStringLiteral("in_progress");
    case VideoStatus::Completed:
        return QStringLiteral("completed");
    case VideoStatus::Failed:
        return QStringLiteral("failed");
    }
    return QStringLiteral("queued");
}

VideoStatus videoStatusFromString(const QString &value)
{
    if (value == QLatin1String("in_progress"))
        return VideoStatus::InProgress;
    if (value == QLatin1String("completed"))
        return VideoStatus::Completed;
    if (value == QLatin1String("failed"))
        return VideoStatus::Failed;
    return VideoStatus::Queued;
}

QString uploadStatusToString(UploadStatus status)
{
    switch (status) {
    case UploadStatus::Pending:
        return QStringLiteral("pending");
    case UploadStatus::Completed:
        return QStringLiteral("completed");
    case UploadStatus::Cancelled:
        return QStringLiteral("cancelled");
    case UploadStatus::Expired:
        return QStringLiteral("expired");
    }
    return QStringLiteral("pending");
}

UploadStatus uploadStatusFromString(const QString &value)
{
    if (value == QLatin1String("completed"))
        return UploadStatus::Completed;
    if (value == QLatin1String("cancelled"))
        return UploadStatus::Cancelled;
    if (value == QLatin1String("expired"))
        return UploadStatus::Expired;
    return UploadStatus::Pending;
}

QString vectorStoreStatusToString(VectorStoreStatus status)
{
    switch (status) {
    case VectorStoreStatus::InProgress:
        return QStringLiteral("in_progress");
    case VectorStoreStatus::Completed:
        return QStringLiteral("completed");
    case VectorStoreStatus::Expired:
        return QStringLiteral("expired");
    }
    return QStringLiteral("in_progress");
}

VectorStoreStatus vectorStoreStatusFromString(const QString &value)
{
    if (value == QLatin1String("completed"))
        return VectorStoreStatus::Completed;
    if (value == QLatin1String("expired"))
        return VectorStoreStatus::Expired;
    return VectorStoreStatus::InProgress;
}

QString vectorStoreFileStatusToString(VectorStoreFileStatus status)
{
    switch (status) {
    case VectorStoreFileStatus::InProgress:
        return QStringLiteral("in_progress");
    case VectorStoreFileStatus::Completed:
        return QStringLiteral("completed");
    case VectorStoreFileStatus::Cancelled:
        return QStringLiteral("cancelled");
    case VectorStoreFileStatus::Failed:
        return QStringLiteral("failed");
    }
    return QStringLiteral("in_progress");
}

VectorStoreFileStatus vectorStoreFileStatusFromString(const QString &value)
{
    if (value == QLatin1String("completed"))
        return VectorStoreFileStatus::Completed;
    if (value == QLatin1String("cancelled"))
        return VectorStoreFileStatus::Cancelled;
    if (value == QLatin1String("failed"))
        return VectorStoreFileStatus::Failed;
    return VectorStoreFileStatus::InProgress;
}

QString batchStatusToString(BatchStatus status)
{
    switch (status) {
    case BatchStatus::Validating:
        return QStringLiteral("validating");
    case BatchStatus::InProgress:
        return QStringLiteral("in_progress");
    case BatchStatus::Finalizing:
        return QStringLiteral("finalizing");
    case BatchStatus::Completed:
        return QStringLiteral("completed");
    case BatchStatus::Failed:
        return QStringLiteral("failed");
    case BatchStatus::Expired:
        return QStringLiteral("expired");
    case BatchStatus::Cancelling:
        return QStringLiteral("cancelling");
    case BatchStatus::Cancelled:
        return QStringLiteral("cancelled");
    }
    return QStringLiteral("validating");
}

BatchStatus batchStatusFromString(const QString &value)
{
    if (value == QLatin1String("in_progress"))
        return BatchStatus::InProgress;
    if (value == QLatin1String("finalizing"))
        return BatchStatus::Finalizing;
    if (value == QLatin1String("completed"))
        return BatchStatus::Completed;
    if (value == QLatin1String("failed"))
        return BatchStatus::Failed;
    if (value == QLatin1String("expired"))
        return BatchStatus::Expired;
    if (value == QLatin1String("cancelling"))
        return BatchStatus::Cancelling;
    if (value == QLatin1String("cancelled"))
        return BatchStatus::Cancelled;
    return BatchStatus::Validating;
}

QString fineTuningJobStatusToString(FineTuningJobStatus status)
{
    switch (status) {
    case FineTuningJobStatus::ValidatingFiles:
        return QStringLiteral("validating_files");
    case FineTuningJobStatus::Queued:
        return QStringLiteral("queued");
    case FineTuningJobStatus::Running:
        return QStringLiteral("running");
    case FineTuningJobStatus::Succeeded:
        return QStringLiteral("succeeded");
    case FineTuningJobStatus::Failed:
        return QStringLiteral("failed");
    case FineTuningJobStatus::Cancelled:
        return QStringLiteral("cancelled");
    case FineTuningJobStatus::Paused:
        return QStringLiteral("paused");
    }
    return QStringLiteral("queued");
}

FineTuningJobStatus fineTuningJobStatusFromString(const QString &value)
{
    if (value == QLatin1String("validating_files"))
        return FineTuningJobStatus::ValidatingFiles;
    if (value == QLatin1String("running"))
        return FineTuningJobStatus::Running;
    if (value == QLatin1String("succeeded"))
        return FineTuningJobStatus::Succeeded;
    if (value == QLatin1String("failed"))
        return FineTuningJobStatus::Failed;
    if (value == QLatin1String("cancelled"))
        return FineTuningJobStatus::Cancelled;
    if (value == QLatin1String("paused"))
        return FineTuningJobStatus::Paused;
    return FineTuningJobStatus::Queued;
}

} // namespace Core
} // namespace QtOpenAi
