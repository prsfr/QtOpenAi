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

} // namespace Core
} // namespace QtOpenAi
