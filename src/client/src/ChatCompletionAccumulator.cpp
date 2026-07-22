// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ChatCompletionAccumulator.h"

#include <QtCore/QMap>

namespace QtOpenAi {
namespace Client {

using namespace QtOpenAi::Core;

namespace {

// Mutable per-tool-call reconstruction state.
struct ToolCallState
{
    QString id;
    QString type = QStringLiteral("function");
    QString name;
    QString arguments;
};

// Mutable per-choice reconstruction state.
struct ChoiceState
{
    Role role = Role::Assistant;
    QString content;
    bool hasContent = false;
    FinishReason finishReason = FinishReason::None;
    QString refusal;
    QMap<int, ToolCallState> toolCalls; // keyed by streamed tool-call index
};

} // namespace

class ChatCompletionAccumulatorPrivate
{
public:
    QString id;
    QString object = QStringLiteral("chat.completion");
    qint64 created = 0;
    QString model;
    QString systemFingerprint;
    QMap<int, ChoiceState> choices; // keyed by choice index
};

ChatCompletionAccumulator::ChatCompletionAccumulator()
    : d(new ChatCompletionAccumulatorPrivate)
{ }

ChatCompletionAccumulator::~ChatCompletionAccumulator() = default;

void ChatCompletionAccumulator::add(const ChatCompletionChunk &chunk)
{
    // Header fields: keep the first non-empty value seen.
    if (d->id.isEmpty())
        d->id = chunk.id();
    if (d->created == 0)
        d->created = chunk.created();
    if (d->model.isEmpty())
        d->model = chunk.model();
    if (d->systemFingerprint.isEmpty())
        d->systemFingerprint = chunk.systemFingerprint();

    for (const ChunkChoice &chunkChoice : chunk.choices()) {
        ChoiceState &state = d->choices[chunkChoice.index()];
        const ChoiceDelta delta = chunkChoice.delta();

        if (delta.role())
            state.role = *delta.role();
        if (delta.hasContent()) {
            state.content += delta.content();
            state.hasContent = true;
        }
        if (!delta.refusal().isEmpty())
            state.refusal += delta.refusal();
        if (chunkChoice.finishReason() != FinishReason::None)
            state.finishReason = chunkChoice.finishReason();

        for (const ToolCallChunk &tc : delta.toolCalls()) {
            ToolCallState &call = state.toolCalls[tc.index()];
            if (!tc.id().isEmpty())
                call.id = tc.id();
            if (!tc.type().isEmpty())
                call.type = tc.type();
            if (!tc.functionName().isEmpty())
                call.name = tc.functionName();
            call.arguments += tc.argumentsFragment();
        }
    }
}

Core::ChatCompletionResponse ChatCompletionAccumulator::response() const
{
    ChatCompletionResponse response;
    response.setId(d->id);
    response.setObject(d->object);
    response.setCreated(d->created);
    response.setModel(d->model);
    response.setSystemFingerprint(d->systemFingerprint);

    QList<Choice> choices;
    for (auto it = d->choices.constBegin(); it != d->choices.constEnd(); ++it) {
        const ChoiceState &state = it.value();

        Message message;
        message.setRole(state.role);
        if (state.hasContent)
            message.setContent(state.content);
        if (!state.refusal.isEmpty())
            message.setRefusal(state.refusal);

        QList<ToolCall> toolCalls;
        for (auto tcIt = state.toolCalls.constBegin(); tcIt != state.toolCalls.constEnd(); ++tcIt) {
            const ToolCallState &call = tcIt.value();
            ToolCall toolCall(call.id, FunctionCall(call.name, call.arguments));
            toolCall.setType(call.type);
            toolCalls.append(toolCall);
        }
        if (!toolCalls.isEmpty())
            message.setToolCalls(toolCalls);

        Choice choice;
        choice.setIndex(it.key());
        choice.setMessage(message);
        choice.setFinishReason(state.finishReason);
        choices.append(choice);
    }
    response.setChoices(choices);
    return response;
}

QString ChatCompletionAccumulator::content(int choiceIndex) const
{
    const auto it = d->choices.constFind(choiceIndex);
    return it == d->choices.constEnd() ? QString() : it.value().content;
}

void ChatCompletionAccumulator::clear() { d.reset(new ChatCompletionAccumulatorPrivate); }

} // namespace Client
} // namespace QtOpenAi
