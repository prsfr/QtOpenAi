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

} // namespace Core
} // namespace QtOpenAi
