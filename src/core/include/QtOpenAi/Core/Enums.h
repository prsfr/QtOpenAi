// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QObject>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

// Q_NAMESPACE exposes the enums below to the Qt meta-object system so they can
// be used with QMetaEnum, queried by name, and streamed in signals/slots.
Q_NAMESPACE_EXPORT(QTOPENAI_CORE_EXPORT)

// The author of a chat message. Mirrors the OpenAI `role` field.
enum class Role {
    System,
    User,
    Assistant,
    Tool,
    Developer,
};
Q_ENUM_NS(Role)

// The reason the model stopped generating tokens. Mirrors `finish_reason`.
enum class FinishReason {
    None,
    Stop,
    Length,
    ToolCalls,
    ContentFilter,
    FunctionCall,
};
Q_ENUM_NS(FinishReason)

// The lifecycle state of a video-generation job (Sora). Mirrors the OpenAI
// `status` field. A job is terminal once it reaches Completed or Failed.
enum class VideoStatus {
    Queued,
    InProgress,
    Completed,
    Failed,
};
Q_ENUM_NS(VideoStatus)

// The lifecycle state of a multipart upload (Uploads API). Mirrors the OpenAI
// `status` field. An upload is terminal once it reaches Completed, Cancelled or
// Expired; only a Pending upload accepts further parts.
enum class UploadStatus {
    Pending,
    Completed,
    Cancelled,
    Expired,
};
Q_ENUM_NS(UploadStatus)

// The lifecycle state of a vector store (Vector Stores API). Mirrors the OpenAI
// `status` field: a store is ready for search once it is Completed.
enum class VectorStoreStatus {
    InProgress,
    Completed,
    Expired,
};
Q_ENUM_NS(VectorStoreStatus)

// The ingestion state of a single file in a vector store — and of a whole file
// batch, which the API reports with the same set of values.
enum class VectorStoreFileStatus {
    InProgress,
    Completed,
    Cancelled,
    Failed,
};
Q_ENUM_NS(VectorStoreFileStatus)

// The lifecycle state of a batch job (Batch API). Mirrors the OpenAI `status`
// field. A batch is terminal once it reaches Completed, Failed, Expired or
// Cancelled; Validating, InProgress, Finalizing and Cancelling are transient.
enum class BatchStatus {
    Validating,
    InProgress,
    Finalizing,
    Completed,
    Failed,
    Expired,
    Cancelling,
    Cancelled,
};
Q_ENUM_NS(BatchStatus)

// The lifecycle state of a fine-tuning job (Fine-tuning API). Mirrors the OpenAI
// `status` field. A job is terminal once it reaches Succeeded, Failed or
// Cancelled; ValidatingFiles, Queued, Running and Paused are transient — a
// paused job resumes on request.
enum class FineTuningJobStatus {
    ValidatingFiles,
    Queued,
    Running,
    Succeeded,
    Failed,
    Cancelled,
    Paused,
};
Q_ENUM_NS(FineTuningJobStatus)

// The lifecycle state of an eval run (Evals API). Mirrors the OpenAI `status`
// field. A run is terminal once it reaches Completed, Failed or Canceled;
// Queued and InProgress are transient. Note the single-l "canceled" spelling
// this endpoint family uses.
enum class EvalRunStatus {
    Queued,
    InProgress,
    Completed,
    Failed,
    Canceled,
};
Q_ENUM_NS(EvalRunStatus)

// The lifecycle state of an assistant run (Assistants API). Mirrors the OpenAI
// `status` field. A run is terminal once it reaches Completed, Failed,
// Cancelled, Incomplete or Expired; Queued, InProgress and Cancelling are
// transient. RequiresAction is the one state that is neither: the run is parked
// until the client submits the outputs of the tool calls it asked for.
enum class RunStatus {
    Queued,
    InProgress,
    RequiresAction,
    Cancelling,
    Cancelled,
    Failed,
    Completed,
    Incomplete,
    Expired,
};
Q_ENUM_NS(RunStatus)

// The lifecycle state of a single step of a run (Assistants API). A step is
// terminal once it reaches Cancelled, Failed, Completed or Expired.
enum class RunStepStatus {
    InProgress,
    Cancelled,
    Failed,
    Completed,
    Expired,
};
Q_ENUM_NS(RunStepStatus)

// The lifecycle state of a ChatKit session. A cancelled or expired session
// stops authenticating requests made with its client secret; nothing polls it,
// so no state here is treated as terminal.
enum class ChatKitSessionStatus {
    Active,
    Expired,
    Cancelled,
};
Q_ENUM_NS(ChatKitSessionStatus)

// The state of a ChatKit thread. Unlike every other status in this file it
// arrives as a tagged object ({"type": "locked", "reason": ...}) rather than a
// bare string; ChatKitThread splits it into this enum plus the reason.
enum class ChatKitThreadStatus {
    Active,
    Locked,
    Closed,
};
Q_ENUM_NS(ChatKitThreadStatus)

// Convert a Role to/from its OpenAI wire representation.
QTOPENAI_CORE_EXPORT QString roleToString(Role role);
QTOPENAI_CORE_EXPORT Role roleFromString(const QString &value);

// Convert a FinishReason to/from its OpenAI wire representation.
// FinishReason::None maps to an empty string (field absent).
QTOPENAI_CORE_EXPORT QString finishReasonToString(FinishReason reason);
QTOPENAI_CORE_EXPORT FinishReason finishReasonFromString(const QString &value);

// Convert a VideoStatus to/from its OpenAI wire representation. An unrecognised
// value decodes to Queued (the initial, non-terminal state).
QTOPENAI_CORE_EXPORT QString videoStatusToString(VideoStatus status);
QTOPENAI_CORE_EXPORT VideoStatus videoStatusFromString(const QString &value);

// Convert an UploadStatus to/from its OpenAI wire representation. An
// unrecognised value decodes to Pending (the initial, non-terminal state).
QTOPENAI_CORE_EXPORT QString uploadStatusToString(UploadStatus status);
QTOPENAI_CORE_EXPORT UploadStatus uploadStatusFromString(const QString &value);

// Convert a VectorStoreStatus to/from its OpenAI wire representation. An
// unrecognised value decodes to InProgress (the initial state).
QTOPENAI_CORE_EXPORT QString vectorStoreStatusToString(VectorStoreStatus status);
QTOPENAI_CORE_EXPORT VectorStoreStatus vectorStoreStatusFromString(const QString &value);

// Convert a VectorStoreFileStatus to/from its OpenAI wire representation. An
// unrecognised value decodes to InProgress (the initial state).
QTOPENAI_CORE_EXPORT QString vectorStoreFileStatusToString(VectorStoreFileStatus status);
QTOPENAI_CORE_EXPORT VectorStoreFileStatus vectorStoreFileStatusFromString(const QString &value);

// Convert a BatchStatus to/from its OpenAI wire representation. An unrecognised
// value decodes to Validating (the initial, non-terminal state), so a client
// polling an unfamiliar status keeps waiting instead of stopping early.
QTOPENAI_CORE_EXPORT QString batchStatusToString(BatchStatus status);
QTOPENAI_CORE_EXPORT BatchStatus batchStatusFromString(const QString &value);

// Convert a FineTuningJobStatus to/from its OpenAI wire representation. An
// unrecognised value decodes to Queued (a transient state), so a client polling
// an unfamiliar status keeps waiting instead of stopping early.
QTOPENAI_CORE_EXPORT QString fineTuningJobStatusToString(FineTuningJobStatus status);
QTOPENAI_CORE_EXPORT FineTuningJobStatus fineTuningJobStatusFromString(const QString &value);

// Convert an EvalRunStatus to/from its OpenAI wire representation. An
// unrecognised value decodes to Queued (the initial, non-terminal state), so a
// client polling an unfamiliar status keeps waiting instead of stopping early.
QTOPENAI_CORE_EXPORT QString evalRunStatusToString(EvalRunStatus status);
QTOPENAI_CORE_EXPORT EvalRunStatus evalRunStatusFromString(const QString &value);

// Convert a RunStatus to/from its OpenAI wire representation. An unrecognised
// value decodes to Queued (the initial, non-terminal state), so a client polling
// an unfamiliar status keeps waiting instead of stopping early.
QTOPENAI_CORE_EXPORT QString runStatusToString(RunStatus status);
QTOPENAI_CORE_EXPORT RunStatus runStatusFromString(const QString &value);

// Convert a RunStepStatus to/from its OpenAI wire representation. An
// unrecognised value decodes to InProgress (the initial, non-terminal state).
QTOPENAI_CORE_EXPORT QString runStepStatusToString(RunStepStatus status);
QTOPENAI_CORE_EXPORT RunStepStatus runStepStatusFromString(const QString &value);

// Convert a ChatKitSessionStatus to/from its OpenAI wire representation. An
// unrecognised value decodes to Active, so a status from a newer server never
// reads as "this session is over".
QTOPENAI_CORE_EXPORT QString chatKitSessionStatusToString(ChatKitSessionStatus status);
QTOPENAI_CORE_EXPORT ChatKitSessionStatus chatKitSessionStatusFromString(const QString &value);

// Convert a ChatKitThreadStatus to/from the `type` of its wire object. An
// unrecognised value decodes to Active, so an unfamiliar state never reads as
// "this thread refuses input".
QTOPENAI_CORE_EXPORT QString chatKitThreadStatusToString(ChatKitThreadStatus status);
QTOPENAI_CORE_EXPORT ChatKitThreadStatus chatKitThreadStatusFromString(const QString &value);

// Whether a status is one its lifecycle will not leave, so a client polling the
// job can stop. These answer for the status alone; the job types wrap them as
// `job.isTerminal()`, which is what callers normally use.
//
// A status the library does not know is never terminal — the same care the
// decode fallbacks take, so an unfamiliar state from a newer server leaves a
// poller waiting instead of stopping it early.
QTOPENAI_CORE_EXPORT bool isTerminal(VideoStatus status);
QTOPENAI_CORE_EXPORT bool isTerminal(UploadStatus status);
QTOPENAI_CORE_EXPORT bool isTerminal(BatchStatus status);
QTOPENAI_CORE_EXPORT bool isTerminal(FineTuningJobStatus status);
QTOPENAI_CORE_EXPORT bool isTerminal(EvalRunStatus status);
QTOPENAI_CORE_EXPORT bool isTerminal(RunStatus status);
QTOPENAI_CORE_EXPORT bool isTerminal(RunStepStatus status);

} // namespace Core
} // namespace QtOpenAi
