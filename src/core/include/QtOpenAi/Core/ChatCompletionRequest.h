// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/Message.h>
#include <QtOpenAi/Core/ResponseFormat.h>
#include <QtOpenAi/Core/Tool.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

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

    // response_format: constrained decoding (text / json_object / json_schema).
    std::optional<ResponseFormat> responseFormat() const;
    void setResponseFormat(const ResponseFormat &format);

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

    // --- Sampling / penalties ----------------------------------------------
    std::optional<double> frequencyPenalty() const;
    void setFrequencyPenalty(double penalty);

    std::optional<double> presencePenalty() const;
    void setPresencePenalty(double penalty);

    // Token-id → bias map; omitted when empty.
    QJsonObject logitBias() const;
    void setLogitBias(const QJsonObject &logitBias);

    std::optional<int> seed() const;
    void setSeed(int seed);

    // stop: a string or an array of up to 4 strings; unset omits the field.
    std::optional<QJsonValue> stop() const;
    void setStop(const QJsonValue &stop);

    std::optional<bool> logprobs() const;
    void setLogprobs(bool logprobs);

    std::optional<int> topLogprobs() const;
    void setTopLogprobs(int topLogprobs);

    // --- Output control ----------------------------------------------------
    // stream_options, e.g. { "include_usage": true }; omitted when empty.
    QJsonObject streamOptions() const;
    void setStreamOptions(const QJsonObject &options);

    // modalities, e.g. ["text", "audio"]; omitted when empty.
    QStringList modalities() const;
    void setModalities(const QStringList &modalities);

    // Predicted outputs object; omitted when empty.
    QJsonObject prediction() const;
    void setPrediction(const QJsonObject &prediction);

    std::optional<bool> parallelToolCalls() const;
    void setParallelToolCalls(bool parallelToolCalls);

    // Deprecated alias for max_completion_tokens.
    std::optional<int> maxTokens() const;
    void setMaxTokens(int tokens);

    // --- Routing / metadata ------------------------------------------------
    QString serviceTier() const;
    void setServiceTier(const QString &serviceTier);

    std::optional<bool> store() const;
    void setStore(bool store);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    QString user() const;
    void setUser(const QString &user);

    QString safetyIdentifier() const;
    void setSafetyIdentifier(const QString &safetyIdentifier);

    QString promptCacheKey() const;
    void setPromptCacheKey(const QString &promptCacheKey);

    // --- Reasoning models --------------------------------------------------
    // "minimal", "low", "medium", "high"; empty omits it.
    QString reasoningEffort() const;
    void setReasoningEffort(const QString &effort);

    // --- Built-in tools ----------------------------------------------------
    // web_search_options object; omitted when empty.
    QJsonObject webSearchOptions() const;
    void setWebSearchOptions(const QJsonObject &options);

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
