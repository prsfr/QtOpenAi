// SPDX-License-Identifier: MIT
#pragma once

// The thirteen fields a run *is*, shared by the `run` resource and the body of
// POST /threads/{id}/runs. The same arrangement as AssistantFields_p.h, and for
// the same reason -- see that file's comment for why the base is private and why
// a public one would be a mistake for these value types.
//
// Run has sixteen more of its own (id, status, the timestamps, last_error,
// usage...), all server-assigned; CreateRunRequest has four of its own
// (additionalInstructions, additionalMessages, thread, stream), all genuinely
// create-only. Those stay where they are. What was duplicated is the middle.
//
// Not installed.

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QSharedData>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {
namespace detail {

class RunFieldsData : public QSharedData
{
public:
    QString assistantId;
    QString model;
    QString instructions;
    QJsonArray tools;
    QJsonObject metadata;
    std::optional<double> temperature;
    std::optional<double> topP;
    std::optional<int> maxPromptTokens;
    std::optional<int> maxCompletionTokens;
    QJsonObject truncationStrategy;
    QJsonValue toolChoice = QJsonValue::Undefined;
    std::optional<bool> parallelToolCalls;
    QJsonValue responseFormat = QJsonValue::Undefined;
};

// The one place these thirteen fields' wire form is written down. QJsonObject
// keeps its keys sorted, so emitting them here rather than interleaved with each
// type's own fields changes nothing about the result.
inline void insertRunFields(QJsonObject &json, const RunFieldsData &d)
{
    insertIfNotEmpty(json, QStringLiteral("assistant_id"), d.assistantId);
    insertIfNotEmpty(json, QStringLiteral("model"), d.model);
    insertIfNotEmpty(json, QStringLiteral("instructions"), d.instructions);
    if (!d.tools.isEmpty())
        json.insert(QStringLiteral("tools"), d.tools);
    if (!d.metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d.metadata);
    insertIfSet(json, QStringLiteral("temperature"), d.temperature);
    insertIfSet(json, QStringLiteral("top_p"), d.topP);
    insertIfSet(json, QStringLiteral("max_prompt_tokens"), d.maxPromptTokens);
    insertIfSet(json, QStringLiteral("max_completion_tokens"), d.maxCompletionTokens);
    if (!d.truncationStrategy.isEmpty())
        json.insert(QStringLiteral("truncation_strategy"), d.truncationStrategy);
    if (!d.toolChoice.isUndefined())
        json.insert(QStringLiteral("tool_choice"), d.toolChoice);
    insertIfSet(json, QStringLiteral("parallel_tool_calls"), d.parallelToolCalls);
    if (!d.responseFormat.isUndefined())
        json.insert(QStringLiteral("response_format"), d.responseFormat);
}

inline void readRunFields(RunFieldsData &d, const QJsonObject &json)
{
    d.assistantId = stringOr(json, QStringLiteral("assistant_id"));
    d.model = stringOr(json, QStringLiteral("model"));
    d.instructions = stringOr(json, QStringLiteral("instructions"));
    d.tools = json.value(QStringLiteral("tools")).toArray();
    d.metadata = json.value(QStringLiteral("metadata")).toObject();
    d.temperature = optionalDouble(json, QStringLiteral("temperature"));
    d.topP = optionalDouble(json, QStringLiteral("top_p"));
    d.maxPromptTokens = optionalInt(json, QStringLiteral("max_prompt_tokens"));
    d.maxCompletionTokens = optionalInt(json, QStringLiteral("max_completion_tokens"));
    d.truncationStrategy = json.value(QStringLiteral("truncation_strategy")).toObject();
    const QJsonValue choice = json.value(QStringLiteral("tool_choice"));
    d.toolChoice = choice.isNull() ? QJsonValue(QJsonValue::Undefined) : choice;
    d.parallelToolCalls = optionalBool(json, QStringLiteral("parallel_tool_calls"));
    const QJsonValue format = json.value(QStringLiteral("response_format"));
    d.responseFormat = format.isNull() ? QJsonValue(QJsonValue::Undefined) : format;
}

inline bool runFieldsEqual(const RunFieldsData &a, const RunFieldsData &b)
{
    return a.assistantId == b.assistantId && a.model == b.model && a.instructions == b.instructions
           && a.tools == b.tools && a.metadata == b.metadata && a.temperature == b.temperature
           && a.topP == b.topP && a.maxPromptTokens == b.maxPromptTokens
           && a.maxCompletionTokens == b.maxCompletionTokens
           && a.truncationStrategy == b.truncationStrategy && a.toolChoice == b.toolChoice
           && a.parallelToolCalls == b.parallelToolCalls && a.responseFormat == b.responseFormat;
}

} // namespace detail
} // namespace Core
} // namespace QtOpenAi
