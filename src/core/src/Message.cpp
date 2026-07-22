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
    QString name;
    QList<ToolCall> toolCalls;
    QString toolCallId;
    QString refusal;
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

QString Message::content() const { return d->content; }
void Message::setContent(const QString &content)
{
    d->content = content;
    d->contentSet = true;
}
bool Message::hasContent() const { return d->contentSet; }

QString Message::name() const { return d->name; }
void Message::setName(const QString &name) { d->name = name; }

QList<ToolCall> Message::toolCalls() const { return d->toolCalls; }
void Message::setToolCalls(const QList<ToolCall> &toolCalls) { d->toolCalls = toolCalls; }
void Message::addToolCall(const ToolCall &toolCall) { d->toolCalls.append(toolCall); }

QString Message::toolCallId() const { return d->toolCallId; }
void Message::setToolCallId(const QString &toolCallId) { d->toolCallId = toolCallId; }

QString Message::refusal() const { return d->refusal; }
void Message::setRefusal(const QString &refusal) { d->refusal = refusal; }

QJsonObject Message::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("role"), roleToString(d->role));

    // content is required by the schema but may legitimately be null for an
    // assistant message that only produced tool calls.
    if (d->contentSet || d->toolCalls.isEmpty())
        json.insert(QStringLiteral("content"), d->content);
    else
        json.insert(QStringLiteral("content"), QJsonValue::Null);

    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNotEmpty(json, QStringLiteral("tool_call_id"), d->toolCallId);
    detail::insertIfNotEmpty(json, QStringLiteral("refusal"), d->refusal);

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
    }

    message.d->name = detail::stringOr(json, QStringLiteral("name"));
    message.d->toolCallId = detail::stringOr(json, QStringLiteral("tool_call_id"));
    message.d->refusal = detail::stringOr(json, QStringLiteral("refusal"));

    const QJsonArray calls = json.value(QStringLiteral("tool_calls")).toArray();
    for (const QJsonValue &value : calls)
        message.d->toolCalls.append(ToolCall::fromJson(value.toObject()));

    return message;
}

Message Message::system(const QString &content) { return Message(Role::System, content); }
Message Message::user(const QString &content) { return Message(Role::User, content); }
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
           && d->contentSet == other.d->contentSet && d->name == other.d->name
           && d->toolCalls == other.d->toolCalls && d->toolCallId == other.d->toolCallId
           && d->refusal == other.d->refusal;
}

} // namespace Core
} // namespace QtOpenAi
