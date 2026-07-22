// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ChatCompletionResponse.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ChatCompletionResponseData : public QSharedData
{
public:
    QString id;
    QString object = QStringLiteral("chat.completion");
    qint64 created = 0;
    QString model;
    QString systemFingerprint;
    QList<Choice> choices;
    Usage usage;
};

ChatCompletionResponse::ChatCompletionResponse()
    : d(new ChatCompletionResponseData)
{
}

ChatCompletionResponse::ChatCompletionResponse(const ChatCompletionResponse &other) = default;
ChatCompletionResponse::ChatCompletionResponse(ChatCompletionResponse &&other) noexcept = default;
ChatCompletionResponse &ChatCompletionResponse::operator=(const ChatCompletionResponse &other) = default;
ChatCompletionResponse &ChatCompletionResponse::operator=(ChatCompletionResponse &&other) noexcept = default;
ChatCompletionResponse::~ChatCompletionResponse() = default;

QString ChatCompletionResponse::id() const { return d->id; }
void ChatCompletionResponse::setId(const QString &id) { d->id = id; }

QString ChatCompletionResponse::object() const { return d->object; }
void ChatCompletionResponse::setObject(const QString &object) { d->object = object; }

qint64 ChatCompletionResponse::created() const { return d->created; }
void ChatCompletionResponse::setCreated(qint64 created) { d->created = created; }

QString ChatCompletionResponse::model() const { return d->model; }
void ChatCompletionResponse::setModel(const QString &model) { d->model = model; }

QString ChatCompletionResponse::systemFingerprint() const { return d->systemFingerprint; }
void ChatCompletionResponse::setSystemFingerprint(const QString &fingerprint)
{
    d->systemFingerprint = fingerprint;
}

QList<Choice> ChatCompletionResponse::choices() const { return d->choices; }
void ChatCompletionResponse::setChoices(const QList<Choice> &choices) { d->choices = choices; }

Usage ChatCompletionResponse::usage() const { return d->usage; }
void ChatCompletionResponse::setUsage(const Usage &usage) { d->usage = usage; }

Message ChatCompletionResponse::firstMessage() const
{
    return d->choices.isEmpty() ? Message() : d->choices.first().message();
}

QList<ToolCall> ChatCompletionResponse::toolCalls() const
{
    return d->choices.isEmpty() ? QList<ToolCall>() : d->choices.first().message().toolCalls();
}

QJsonObject ChatCompletionResponse::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("id"), d->id);
    json.insert(QStringLiteral("object"), d->object);
    json.insert(QStringLiteral("created"), d->created);
    json.insert(QStringLiteral("model"), d->model);
    detail::insertIfNotEmpty(json, QStringLiteral("system_fingerprint"), d->systemFingerprint);

    QJsonArray choices;
    for (const Choice &choice : d->choices)
        choices.append(choice.toJson());
    json.insert(QStringLiteral("choices"), choices);

    json.insert(QStringLiteral("usage"), d->usage.toJson());
    return json;
}

ChatCompletionResponse ChatCompletionResponse::fromJson(const QJsonObject &json)
{
    ChatCompletionResponse response;
    response.d->id = detail::stringOr(json, QStringLiteral("id"));
    response.d->object = detail::stringOr(json, QStringLiteral("object"), QStringLiteral("chat.completion"));
    response.d->created = static_cast<qint64>(json.value(QStringLiteral("created")).toDouble());
    response.d->model = detail::stringOr(json, QStringLiteral("model"));
    response.d->systemFingerprint = detail::stringOr(json, QStringLiteral("system_fingerprint"));

    const QJsonArray choices = json.value(QStringLiteral("choices")).toArray();
    for (const QJsonValue &value : choices)
        response.d->choices.append(Choice::fromJson(value.toObject()));

    if (json.contains(QStringLiteral("usage")))
        response.d->usage = Usage::fromJson(json.value(QStringLiteral("usage")).toObject());

    return response;
}

bool ChatCompletionResponse::operator==(const ChatCompletionResponse &other) const
{
    return d->id == other.d->id
        && d->object == other.d->object
        && d->created == other.d->created
        && d->model == other.d->model
        && d->systemFingerprint == other.d->systemFingerprint
        && d->choices == other.d->choices
        && d->usage == other.d->usage;
}

} // namespace Core
} // namespace QtOpenAi
