// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/ContentPart.h>
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

    // Plain-text content. When the message uses structured content parts,
    // content() returns the concatenation of their text parts.
    QString content() const;
    void setContent(const QString &content);
    bool hasContent() const;

    // Structured multimodal content (text/image/audio/file parts). When
    // non-empty, the message serialises `content` as an array; otherwise the
    // plain string form is used, so existing string-only code is unaffected.
    QList<ContentPart> contentParts() const;
    void setContentParts(const QList<ContentPart> &parts);
    void addContentPart(const ContentPart &part);

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

    // Assistant audio output (the response message's `audio` object): a
    // generated id, base64 audio data, and a text transcript. Serialised under
    // `audio` when any field is set.
    QString audioId() const;
    void setAudioId(const QString &audioId);
    QString audioData() const;
    void setAudioData(const QString &audioData);
    QString audioTranscript() const;
    void setAudioTranscript(const QString &transcript);

    QJsonObject toJson() const;
    static Message fromJson(const QJsonObject &json);

    // Named constructors for the common message shapes.
    static Message system(const QString &content);
    static Message user(const QString &content);
    // Multimodal user message from structured content parts.
    static Message user(const QList<ContentPart> &parts);
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
