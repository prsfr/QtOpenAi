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

// Convert a Role to/from its OpenAI wire representation.
QTOPENAI_CORE_EXPORT QString roleToString(Role role);
QTOPENAI_CORE_EXPORT Role roleFromString(const QString &value);

// Convert a FinishReason to/from its OpenAI wire representation.
// FinishReason::None maps to an empty string (field absent).
QTOPENAI_CORE_EXPORT QString finishReasonToString(FinishReason reason);
QTOPENAI_CORE_EXPORT FinishReason finishReasonFromString(const QString &value);

} // namespace Core
} // namespace QtOpenAi
