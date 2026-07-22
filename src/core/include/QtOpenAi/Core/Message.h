// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/Enums.h>
#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ToolCall.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class MessageData;

// A single chat message. Serves both as request input and response output,
// mirroring OpenAI's ChatCompletionRequestMessage / ResponseMessage schemas.
//
// Implicitly shared value type; the implementation is hidden behind a
// QSharedDataPointer d-pointer.
class QTOPENAI_CORE_EXPORT Message
{
public:
    Message();
    Message(Role role, QString content);
    Message(const Message &other);
    Message(Message &&other) noexcept;
    Message &operator=(const Message &other);
    Message &operator=(Message &&other) noexcept;
    ~Message();

    void swap(Message &other) noexcept { d.swap(other.d); }

    Role role() const;
    void setRole(Role role);

    QString content() const;
    void setContent(const QString &content);
    bool hasContent() const;

    // Optional participant name (OpenAI `name` field).
    QString name() const;
    void setName(const QString &name);

    // Tool calls requested by an assistant message.
    QList<ToolCall> toolCalls() const;
    void setToolCalls(const QList<ToolCall> &toolCalls);
    void addToolCall(const ToolCall &toolCall);

    // For tool-result messages: the id of the tool call being answered.
    QString toolCallId() const;
    void setToolCallId(const QString &toolCallId);

    // The model's refusal message, when present.
    QString refusal() const;
    void setRefusal(const QString &refusal);

    QJsonObject toJson() const;
    static Message fromJson(const QJsonObject &json);

    // Named constructors for the common message shapes.
    static Message system(const QString &content);
    static Message user(const QString &content);
    static Message assistant(const QString &content);
    static Message toolResult(const QString &toolCallId, const QString &content);

    bool operator==(const Message &other) const;
    bool operator!=(const Message &other) const { return !(*this == other); }

private:
    QSharedDataPointer<MessageData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Message)
