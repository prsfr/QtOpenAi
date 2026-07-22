// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/Message.h>
#include <QtOpenAi/Core/Tool.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {

class ChatCompletionRequestData;

// The body of a POST /chat/completions request. Optional parameters are only
// serialised when explicitly set, matching the OpenAI schema semantics.
class QTOPENAI_CORE_EXPORT ChatCompletionRequest
{
public:
    ChatCompletionRequest();
    ChatCompletionRequest(QString model, QList<Message> messages);
    ChatCompletionRequest(const ChatCompletionRequest &other);
    ChatCompletionRequest(ChatCompletionRequest &&other) noexcept;
    ChatCompletionRequest &operator=(const ChatCompletionRequest &other);
    ChatCompletionRequest &operator=(ChatCompletionRequest &&other) noexcept;
    ~ChatCompletionRequest();

    void swap(ChatCompletionRequest &other) noexcept { d.swap(other.d); }

    QString model() const;
    void setModel(const QString &model);

    QList<Message> messages() const;
    void setMessages(const QList<Message> &messages);
    void addMessage(const Message &message);

    QList<Tool> tools() const;
    void setTools(const QList<Tool> &tools);
    void addTool(const Tool &tool);

    // tool_choice: "auto", "none", "required", or a specific tool object.
    // Stored as an opaque JSON value; unset means the field is omitted.
    std::optional<QJsonValue> toolChoice() const;
    void setToolChoice(const QJsonValue &toolChoice);

    std::optional<double> temperature() const;
    void setTemperature(double temperature);

    std::optional<double> topP() const;
    void setTopP(double topP);

    std::optional<int> maxCompletionTokens() const;
    void setMaxCompletionTokens(int tokens);

    std::optional<int> n() const;
    void setN(int n);

    std::optional<bool> stream() const;
    void setStream(bool stream);

    // Extra provider-specific fields merged verbatim into the request body.
    QJsonObject extraBody() const;
    void setExtraBody(const QJsonObject &extra);

    QJsonObject toJson() const;
    static ChatCompletionRequest fromJson(const QJsonObject &json);

    bool operator==(const ChatCompletionRequest &other) const;
    bool operator!=(const ChatCompletionRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ChatCompletionRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ChatCompletionRequest)
