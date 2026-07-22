// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ChatCompletionChunk.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

// ---------------------------------------------------------------------------
// ToolCallChunk
// ---------------------------------------------------------------------------

class ToolCallChunkData : public QSharedData
{
public:
    int index = 0;
    QString id;
    QString type;
    QString functionName;
    QString argumentsFragment;
};

ToolCallChunk::ToolCallChunk()
    : d(new ToolCallChunkData)
{ }

ToolCallChunk::ToolCallChunk(const ToolCallChunk &other) = default;
ToolCallChunk::ToolCallChunk(ToolCallChunk &&other) noexcept = default;
ToolCallChunk &ToolCallChunk::operator=(const ToolCallChunk &other) = default;
ToolCallChunk &ToolCallChunk::operator=(ToolCallChunk &&other) noexcept = default;
ToolCallChunk::~ToolCallChunk() = default;

int ToolCallChunk::index() const { return d->index; }
void ToolCallChunk::setIndex(int index) { d->index = index; }

QString ToolCallChunk::id() const { return d->id; }
void ToolCallChunk::setId(const QString &id) { d->id = id; }

QString ToolCallChunk::type() const { return d->type; }
void ToolCallChunk::setType(const QString &type) { d->type = type; }

QString ToolCallChunk::functionName() const { return d->functionName; }
void ToolCallChunk::setFunctionName(const QString &name) { d->functionName = name; }

QString ToolCallChunk::argumentsFragment() const { return d->argumentsFragment; }
void ToolCallChunk::setArgumentsFragment(const QString &fragment)
{
    d->argumentsFragment = fragment;
}

QJsonObject ToolCallChunk::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("index"), d->index);
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("type"), d->type);

    QJsonObject function;
    detail::insertIfNotEmpty(function, QStringLiteral("name"), d->functionName);
    // The arguments fragment is emitted whenever present (even if empty is not).
    if (!d->argumentsFragment.isEmpty())
        function.insert(QStringLiteral("arguments"), d->argumentsFragment);
    if (!function.isEmpty())
        json.insert(QStringLiteral("function"), function);
    return json;
}

ToolCallChunk ToolCallChunk::fromJson(const QJsonObject &json)
{
    ToolCallChunk chunk;
    chunk.d->index = json.value(QStringLiteral("index")).toInt();
    chunk.d->id = detail::stringOr(json, QStringLiteral("id"));
    chunk.d->type = detail::stringOr(json, QStringLiteral("type"));

    const QJsonObject function = json.value(QStringLiteral("function")).toObject();
    chunk.d->functionName = detail::stringOr(function, QStringLiteral("name"));
    chunk.d->argumentsFragment = detail::stringOr(function, QStringLiteral("arguments"));
    return chunk;
}

bool ToolCallChunk::operator==(const ToolCallChunk &other) const
{
    return d->index == other.d->index && d->id == other.d->id && d->type == other.d->type
           && d->functionName == other.d->functionName
           && d->argumentsFragment == other.d->argumentsFragment;
}

// ---------------------------------------------------------------------------
// ChoiceDelta
// ---------------------------------------------------------------------------

class ChoiceDeltaData : public QSharedData
{
public:
    std::optional<Role> role;
    QString content;
    bool contentSet = false;
    QList<ToolCallChunk> toolCalls;
    QString refusal;
};

ChoiceDelta::ChoiceDelta()
    : d(new ChoiceDeltaData)
{ }

ChoiceDelta::ChoiceDelta(const ChoiceDelta &other) = default;
ChoiceDelta::ChoiceDelta(ChoiceDelta &&other) noexcept = default;
ChoiceDelta &ChoiceDelta::operator=(const ChoiceDelta &other) = default;
ChoiceDelta &ChoiceDelta::operator=(ChoiceDelta &&other) noexcept = default;
ChoiceDelta::~ChoiceDelta() = default;

std::optional<Role> ChoiceDelta::role() const { return d->role; }
void ChoiceDelta::setRole(Role role) { d->role = role; }

QString ChoiceDelta::content() const { return d->content; }
void ChoiceDelta::setContent(const QString &content)
{
    d->content = content;
    d->contentSet = true;
}
bool ChoiceDelta::hasContent() const { return d->contentSet; }

QList<ToolCallChunk> ChoiceDelta::toolCalls() const { return d->toolCalls; }
void ChoiceDelta::setToolCalls(const QList<ToolCallChunk> &toolCalls) { d->toolCalls = toolCalls; }

QString ChoiceDelta::refusal() const { return d->refusal; }
void ChoiceDelta::setRefusal(const QString &refusal) { d->refusal = refusal; }

QJsonObject ChoiceDelta::toJson() const
{
    QJsonObject json;
    if (d->role)
        json.insert(QStringLiteral("role"), roleToString(*d->role));
    if (d->contentSet)
        json.insert(QStringLiteral("content"), d->content);
    detail::insertIfNotEmpty(json, QStringLiteral("refusal"), d->refusal);
    if (!d->toolCalls.isEmpty()) {
        QJsonArray array;
        for (const ToolCallChunk &chunk : d->toolCalls)
            array.append(chunk.toJson());
        json.insert(QStringLiteral("tool_calls"), array);
    }
    return json;
}

ChoiceDelta ChoiceDelta::fromJson(const QJsonObject &json)
{
    ChoiceDelta delta;
    if (json.contains(QStringLiteral("role")) && json.value(QStringLiteral("role")).isString()) {
        delta.d->role = roleFromString(json.value(QStringLiteral("role")).toString());
    }
    const QJsonValue content = json.value(QStringLiteral("content"));
    if (content.isString()) {
        delta.d->content = content.toString();
        delta.d->contentSet = true;
    }
    delta.d->refusal = detail::stringOr(json, QStringLiteral("refusal"));

    const QJsonArray toolCalls = json.value(QStringLiteral("tool_calls")).toArray();
    for (const QJsonValue &value : toolCalls)
        delta.d->toolCalls.append(ToolCallChunk::fromJson(value.toObject()));
    return delta;
}

bool ChoiceDelta::operator==(const ChoiceDelta &other) const
{
    return d->role == other.d->role && d->content == other.d->content
           && d->contentSet == other.d->contentSet && d->toolCalls == other.d->toolCalls
           && d->refusal == other.d->refusal;
}

// ---------------------------------------------------------------------------
// ChunkChoice
// ---------------------------------------------------------------------------

class ChunkChoiceData : public QSharedData
{
public:
    int index = 0;
    ChoiceDelta delta;
    FinishReason finishReason = FinishReason::None;
};

ChunkChoice::ChunkChoice()
    : d(new ChunkChoiceData)
{ }

ChunkChoice::ChunkChoice(const ChunkChoice &other) = default;
ChunkChoice::ChunkChoice(ChunkChoice &&other) noexcept = default;
ChunkChoice &ChunkChoice::operator=(const ChunkChoice &other) = default;
ChunkChoice &ChunkChoice::operator=(ChunkChoice &&other) noexcept = default;
ChunkChoice::~ChunkChoice() = default;

int ChunkChoice::index() const { return d->index; }
void ChunkChoice::setIndex(int index) { d->index = index; }

ChoiceDelta ChunkChoice::delta() const { return d->delta; }
void ChunkChoice::setDelta(const ChoiceDelta &delta) { d->delta = delta; }

FinishReason ChunkChoice::finishReason() const { return d->finishReason; }
void ChunkChoice::setFinishReason(FinishReason reason) { d->finishReason = reason; }

QJsonObject ChunkChoice::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("index"), d->index);
    json.insert(QStringLiteral("delta"), d->delta.toJson());
    const QString reason = finishReasonToString(d->finishReason);
    json.insert(QStringLiteral("finish_reason"),
                reason.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(reason));
    return json;
}

ChunkChoice ChunkChoice::fromJson(const QJsonObject &json)
{
    ChunkChoice choice;
    choice.d->index = json.value(QStringLiteral("index")).toInt();
    choice.d->delta = ChoiceDelta::fromJson(json.value(QStringLiteral("delta")).toObject());
    choice.d->finishReason
            = finishReasonFromString(detail::stringOr(json, QStringLiteral("finish_reason")));
    return choice;
}

bool ChunkChoice::operator==(const ChunkChoice &other) const
{
    return d->index == other.d->index && d->delta == other.d->delta
           && d->finishReason == other.d->finishReason;
}

// ---------------------------------------------------------------------------
// ChatCompletionChunk
// ---------------------------------------------------------------------------

class ChatCompletionChunkData : public QSharedData
{
public:
    QString id;
    QString object = QStringLiteral("chat.completion.chunk");
    qint64 created = 0;
    QString model;
    QString systemFingerprint;
    QList<ChunkChoice> choices;
};

ChatCompletionChunk::ChatCompletionChunk()
    : d(new ChatCompletionChunkData)
{ }

ChatCompletionChunk::ChatCompletionChunk(const ChatCompletionChunk &other) = default;
ChatCompletionChunk::ChatCompletionChunk(ChatCompletionChunk &&other) noexcept = default;
ChatCompletionChunk &ChatCompletionChunk::operator=(const ChatCompletionChunk &other) = default;
ChatCompletionChunk &ChatCompletionChunk::operator=(ChatCompletionChunk &&other) noexcept = default;
ChatCompletionChunk::~ChatCompletionChunk() = default;

QString ChatCompletionChunk::id() const { return d->id; }
void ChatCompletionChunk::setId(const QString &id) { d->id = id; }

QString ChatCompletionChunk::object() const { return d->object; }
void ChatCompletionChunk::setObject(const QString &object) { d->object = object; }

qint64 ChatCompletionChunk::created() const { return d->created; }
void ChatCompletionChunk::setCreated(qint64 created) { d->created = created; }

QString ChatCompletionChunk::model() const { return d->model; }
void ChatCompletionChunk::setModel(const QString &model) { d->model = model; }

QString ChatCompletionChunk::systemFingerprint() const { return d->systemFingerprint; }
void ChatCompletionChunk::setSystemFingerprint(const QString &fingerprint)
{
    d->systemFingerprint = fingerprint;
}

QList<ChunkChoice> ChatCompletionChunk::choices() const { return d->choices; }
void ChatCompletionChunk::setChoices(const QList<ChunkChoice> &choices) { d->choices = choices; }

QJsonObject ChatCompletionChunk::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("id"), d->id);
    json.insert(QStringLiteral("object"), d->object);
    json.insert(QStringLiteral("created"), d->created);
    json.insert(QStringLiteral("model"), d->model);
    detail::insertIfNotEmpty(json, QStringLiteral("system_fingerprint"), d->systemFingerprint);

    QJsonArray choices;
    for (const ChunkChoice &choice : d->choices)
        choices.append(choice.toJson());
    json.insert(QStringLiteral("choices"), choices);
    return json;
}

ChatCompletionChunk ChatCompletionChunk::fromJson(const QJsonObject &json)
{
    ChatCompletionChunk chunk;
    chunk.d->id = detail::stringOr(json, QStringLiteral("id"));
    chunk.d->object = detail::stringOr(json, QStringLiteral("object"),
                                       QStringLiteral("chat.completion.chunk"));
    chunk.d->created = static_cast<qint64>(json.value(QStringLiteral("created")).toDouble());
    chunk.d->model = detail::stringOr(json, QStringLiteral("model"));
    chunk.d->systemFingerprint = detail::stringOr(json, QStringLiteral("system_fingerprint"));

    const QJsonArray choices = json.value(QStringLiteral("choices")).toArray();
    for (const QJsonValue &value : choices)
        chunk.d->choices.append(ChunkChoice::fromJson(value.toObject()));
    return chunk;
}

bool ChatCompletionChunk::operator==(const ChatCompletionChunk &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->created == other.d->created
           && d->model == other.d->model && d->systemFingerprint == other.d->systemFingerprint
           && d->choices == other.d->choices;
}

} // namespace Core
} // namespace QtOpenAi
