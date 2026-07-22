// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/Choice.h>
#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/Usage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class ChatCompletionResponseData;

// A parsed `chat.completion` response object.
class QTOPENAI_CORE_EXPORT ChatCompletionResponse
{
public:
    ChatCompletionResponse();
    ChatCompletionResponse(const ChatCompletionResponse &other);
    ChatCompletionResponse(ChatCompletionResponse &&other) noexcept;
    ChatCompletionResponse &operator=(const ChatCompletionResponse &other);
    ChatCompletionResponse &operator=(ChatCompletionResponse &&other) noexcept;
    ~ChatCompletionResponse();

    void swap(ChatCompletionResponse &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    QString object() const;
    void setObject(const QString &object);

    qint64 created() const;
    void setCreated(qint64 created);

    QString model() const;
    void setModel(const QString &model);

    QString systemFingerprint() const;
    void setSystemFingerprint(const QString &fingerprint);

    QList<Choice> choices() const;
    void setChoices(const QList<Choice> &choices);

    Usage usage() const;
    void setUsage(const Usage &usage);

    // Convenience: the first choice's message, or a default-constructed Message.
    Message firstMessage() const;
    // Convenience: tool calls of the first choice, or an empty list.
    QList<ToolCall> toolCalls() const;

    QJsonObject toJson() const;
    static ChatCompletionResponse fromJson(const QJsonObject &json);

    bool operator==(const ChatCompletionResponse &other) const;
    bool operator!=(const ChatCompletionResponse &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ChatCompletionResponseData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ChatCompletionResponse)
