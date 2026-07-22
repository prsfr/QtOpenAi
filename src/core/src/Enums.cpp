// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Enums.h"

namespace QtOpenAi {
namespace Core {

QString roleToString(Role role)
{
    switch (role) {
    case Role::System:    return QStringLiteral("system");
    case Role::User:      return QStringLiteral("user");
    case Role::Assistant: return QStringLiteral("assistant");
    case Role::Tool:      return QStringLiteral("tool");
    case Role::Developer: return QStringLiteral("developer");
    }
    return QStringLiteral("user");
}

Role roleFromString(const QString &value)
{
    if (value == QLatin1String("system"))    return Role::System;
    if (value == QLatin1String("assistant")) return Role::Assistant;
    if (value == QLatin1String("tool"))      return Role::Tool;
    if (value == QLatin1String("developer")) return Role::Developer;
    return Role::User;
}

QString finishReasonToString(FinishReason reason)
{
    switch (reason) {
    case FinishReason::None:          return QString();
    case FinishReason::Stop:          return QStringLiteral("stop");
    case FinishReason::Length:        return QStringLiteral("length");
    case FinishReason::ToolCalls:     return QStringLiteral("tool_calls");
    case FinishReason::ContentFilter: return QStringLiteral("content_filter");
    case FinishReason::FunctionCall:  return QStringLiteral("function_call");
    }
    return QString();
}

FinishReason finishReasonFromString(const QString &value)
{
    if (value == QLatin1String("stop"))           return FinishReason::Stop;
    if (value == QLatin1String("length"))         return FinishReason::Length;
    if (value == QLatin1String("tool_calls"))     return FinishReason::ToolCalls;
    if (value == QLatin1String("content_filter")) return FinishReason::ContentFilter;
    if (value == QLatin1String("function_call"))  return FinishReason::FunctionCall;
    return FinishReason::None;
}

} // namespace Core
} // namespace QtOpenAi
