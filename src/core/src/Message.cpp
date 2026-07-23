// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Message.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class MessageData : public QSharedData
{
public:
    Role role = Role::User;
    QString content;
    bool contentSet = false;
    QList<ContentPart> contentParts;
    QString name;
    QList<ToolCall> toolCalls;
    QString toolCallId;
    QString refusal;
    QString audioId;
    QString audioData;
    QString audioTranscript;
};

Message::Message()
    : d(new MessageData)
{ }

Message::Message(Role role, QString content)
    : d(new MessageData)
{
    d->role = role;
    d->content = std::move(content);
    d->contentSet = true;
}

Message::Message(const Message &other) = default;
Message::Message(Message &&other) noexcept = default;
Message &Message::operator=(const Message &other) = default;
Message &Message::operator=(Message &&other) noexcept = default;
Message::~Message() = default;

Role Message::role() const { return d->role; }
void Message::setRole(Role role) { d->role = role; }

QString Message::content() const
{
    if (!d->contentParts.isEmpty()) {
        QString text;
        for (const ContentPart &part : d->contentParts) {
            if (part.isText())
                text += part.text();
        }
        return text;
    }
    return d->content;
}
void Message::setContent(const QString &content)
{
    d->content = content;
    d->contentSet = true;
}
bool Message::hasContent() const { return d->contentSet || !d->contentParts.isEmpty(); }

QList<ContentPart> Message::contentParts() const { return d->contentParts; }
void Message::setContentParts(const QList<ContentPart> &parts) { d->contentParts = parts; }
void Message::addContentPart(const ContentPart &part) { d->contentParts.append(part); }

QString Message::name() const { return d->name; }
void Message::setName(const QString &name) { d->name = name; }

QList<ToolCall> Message::toolCalls() const { return d->toolCalls; }
void Message::setToolCalls(const QList<ToolCall> &toolCalls) { d->toolCalls = toolCalls; }
void Message::addToolCall(const ToolCall &toolCall) { d->toolCalls.append(toolCall); }

QString Message::toolCallId() const { return d->toolCallId; }
void Message::setToolCallId(const QString &toolCallId) { d->toolCallId = toolCallId; }

QString Message::refusal() const { return d->refusal; }
void Message::setRefusal(const QString &refusal) { d->refusal = refusal; }

QString Message::audioId() const { return d->audioId; }
void Message::setAudioId(const QString &audioId) { d->audioId = audioId; }

QString Message::audioData() const { return d->audioData; }
void Message::setAudioData(const QString &audioData) { d->audioData = audioData; }

QString Message::audioTranscript() const { return d->audioTranscript; }
void Message::setAudioTranscript(const QString &transcript) { d->audioTranscript = transcript; }

QJsonObject Message::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("role"), roleToString(d->role));

    // content may be a structured array of parts, a plain string, or null for an
    // assistant message that only produced tool calls.
    if (!d->contentParts.isEmpty()) {
        QJsonArray parts;
        for (const ContentPart &part : d->contentParts)
            parts.append(part.toJson());
        json.insert(QStringLiteral("content"), parts);
    } else if (d->contentSet || d->toolCalls.isEmpty()) {
        json.insert(QStringLiteral("content"), d->content);
    } else {
        json.insert(QStringLiteral("content"), QJsonValue::Null);
    }

    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNotEmpty(json, QStringLiteral("tool_call_id"), d->toolCallId);
    detail::insertIfNotEmpty(json, QStringLiteral("refusal"), d->refusal);

    if (!d->audioId.isEmpty() || !d->audioData.isEmpty() || !d->audioTranscript.isEmpty()) {
        QJsonObject audio;
        detail::insertIfNotEmpty(audio, QStringLiteral("id"), d->audioId);
        detail::insertIfNotEmpty(audio, QStringLiteral("data"), d->audioData);
        detail::insertIfNotEmpty(audio, QStringLiteral("transcript"), d->audioTranscript);
        json.insert(QStringLiteral("audio"), audio);
    }

    if (!d->toolCalls.isEmpty()) {
        QJsonArray array;
        for (const ToolCall &call : d->toolCalls)
            array.append(call.toJson());
        json.insert(QStringLiteral("tool_calls"), array);
    }
    return json;
}

Message Message::fromJson(const QJsonObject &json)
{
    Message message;
    message.d->role = roleFromString(detail::stringOr(json, QStringLiteral("role")));

    const QJsonValue content = json.value(QStringLiteral("content"));
    if (content.isString()) {
        message.d->content = content.toString();
        message.d->contentSet = true;
    } else if (content.isArray()) {
        const QJsonArray parts = content.toArray();
        for (const QJsonValue &value : parts)
            message.d->contentParts.append(ContentPart::fromJson(value.toObject()));
    }

    message.d->name = detail::stringOr(json, QStringLiteral("name"));
    message.d->toolCallId = detail::stringOr(json, QStringLiteral("tool_call_id"));
    message.d->refusal = detail::stringOr(json, QStringLiteral("refusal"));

    const QJsonObject audio = json.value(QStringLiteral("audio")).toObject();
    message.d->audioId = detail::stringOr(audio, QStringLiteral("id"));
    message.d->audioData = detail::stringOr(audio, QStringLiteral("data"));
    message.d->audioTranscript = detail::stringOr(audio, QStringLiteral("transcript"));

    const QJsonArray calls = json.value(QStringLiteral("tool_calls")).toArray();
    for (const QJsonValue &value : calls)
        message.d->toolCalls.append(ToolCall::fromJson(value.toObject()));

    return message;
}

Message Message::system(const QString &content) { return Message(Role::System, content); }
Message Message::user(const QString &content) { return Message(Role::User, content); }

Message Message::user(const QList<ContentPart> &parts)
{
    Message message;
    message.d->role = Role::User;
    message.d->contentParts = parts;
    return message;
}

Message Message::assistant(const QString &content) { return Message(Role::Assistant, content); }

Message Message::toolResult(const QString &toolCallId, const QString &content)
{
    Message message(Role::Tool, content);
    message.d->toolCallId = toolCallId;
    return message;
}

bool Message::operator==(const Message &other) const
{
    return d->role == other.d->role && d->content == other.d->content
           && d->contentSet == other.d->contentSet && d->contentParts == other.d->contentParts
           && d->name == other.d->name && d->toolCalls == other.d->toolCalls
           && d->toolCallId == other.d->toolCallId && d->refusal == other.d->refusal
           && d->audioId == other.d->audioId && d->audioData == other.d->audioData
           && d->audioTranscript == other.d->audioTranscript;
}

} // namespace Core
} // namespace QtOpenAi
