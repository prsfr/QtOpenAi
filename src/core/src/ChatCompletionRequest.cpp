// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ChatCompletionRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ChatCompletionRequestData : public QSharedData
{
public:
    QString model;
    QList<Message> messages;
    QList<Tool> tools;
    std::optional<QJsonValue> toolChoice;
    std::optional<double> temperature;
    std::optional<double> topP;
    std::optional<int> maxCompletionTokens;
    std::optional<int> n;
    std::optional<bool> stream;
    std::optional<double> frequencyPenalty;
    std::optional<double> presencePenalty;
    QJsonObject logitBias;
    std::optional<int> seed;
    std::optional<QJsonValue> stop;
    std::optional<bool> logprobs;
    std::optional<int> topLogprobs;
    QJsonObject streamOptions;
    QStringList modalities;
    QJsonObject prediction;
    std::optional<bool> parallelToolCalls;
    std::optional<int> maxTokens;
    QString serviceTier;
    std::optional<bool> store;
    QJsonObject metadata;
    QString user;
    QString safetyIdentifier;
    QString promptCacheKey;
    QString reasoningEffort;
    QJsonObject webSearchOptions;
    QJsonObject extraBody;
};

ChatCompletionRequest::ChatCompletionRequest()
    : d(new ChatCompletionRequestData)
{ }

ChatCompletionRequest::ChatCompletionRequest(QString model, QList<Message> messages)
    : d(new ChatCompletionRequestData)
{
    d->model = std::move(model);
    d->messages = std::move(messages);
}

ChatCompletionRequest::ChatCompletionRequest(const ChatCompletionRequest &other) = default;
ChatCompletionRequest::ChatCompletionRequest(ChatCompletionRequest &&other) noexcept = default;
ChatCompletionRequest &ChatCompletionRequest::operator=(const ChatCompletionRequest &other)
        = default;
ChatCompletionRequest &ChatCompletionRequest::operator=(ChatCompletionRequest &&other) noexcept
        = default;
ChatCompletionRequest::~ChatCompletionRequest() = default;

QString ChatCompletionRequest::model() const { return d->model; }
void ChatCompletionRequest::setModel(const QString &model) { d->model = model; }

QList<Message> ChatCompletionRequest::messages() const { return d->messages; }
void ChatCompletionRequest::setMessages(const QList<Message> &messages) { d->messages = messages; }
void ChatCompletionRequest::addMessage(const Message &message) { d->messages.append(message); }

QList<Tool> ChatCompletionRequest::tools() const { return d->tools; }
void ChatCompletionRequest::setTools(const QList<Tool> &tools) { d->tools = tools; }
void ChatCompletionRequest::addTool(const Tool &tool) { d->tools.append(tool); }

std::optional<QJsonValue> ChatCompletionRequest::toolChoice() const { return d->toolChoice; }
void ChatCompletionRequest::setToolChoice(const QJsonValue &toolChoice)
{
    d->toolChoice = toolChoice;
}

std::optional<double> ChatCompletionRequest::temperature() const { return d->temperature; }
void ChatCompletionRequest::setTemperature(double temperature) { d->temperature = temperature; }

std::optional<double> ChatCompletionRequest::topP() const { return d->topP; }
void ChatCompletionRequest::setTopP(double topP) { d->topP = topP; }

std::optional<int> ChatCompletionRequest::maxCompletionTokens() const
{
    return d->maxCompletionTokens;
}
void ChatCompletionRequest::setMaxCompletionTokens(int tokens) { d->maxCompletionTokens = tokens; }

std::optional<int> ChatCompletionRequest::n() const { return d->n; }
void ChatCompletionRequest::setN(int n) { d->n = n; }

std::optional<bool> ChatCompletionRequest::stream() const { return d->stream; }
void ChatCompletionRequest::setStream(bool stream) { d->stream = stream; }

std::optional<double> ChatCompletionRequest::frequencyPenalty() const
{
    return d->frequencyPenalty;
}
void ChatCompletionRequest::setFrequencyPenalty(double penalty) { d->frequencyPenalty = penalty; }

std::optional<double> ChatCompletionRequest::presencePenalty() const { return d->presencePenalty; }
void ChatCompletionRequest::setPresencePenalty(double penalty) { d->presencePenalty = penalty; }

QJsonObject ChatCompletionRequest::logitBias() const { return d->logitBias; }
void ChatCompletionRequest::setLogitBias(const QJsonObject &logitBias) { d->logitBias = logitBias; }

std::optional<int> ChatCompletionRequest::seed() const { return d->seed; }
void ChatCompletionRequest::setSeed(int seed) { d->seed = seed; }

std::optional<QJsonValue> ChatCompletionRequest::stop() const { return d->stop; }
void ChatCompletionRequest::setStop(const QJsonValue &stop) { d->stop = stop; }

std::optional<bool> ChatCompletionRequest::logprobs() const { return d->logprobs; }
void ChatCompletionRequest::setLogprobs(bool logprobs) { d->logprobs = logprobs; }

std::optional<int> ChatCompletionRequest::topLogprobs() const { return d->topLogprobs; }
void ChatCompletionRequest::setTopLogprobs(int topLogprobs) { d->topLogprobs = topLogprobs; }

QJsonObject ChatCompletionRequest::streamOptions() const { return d->streamOptions; }
void ChatCompletionRequest::setStreamOptions(const QJsonObject &options)
{
    d->streamOptions = options;
}

QStringList ChatCompletionRequest::modalities() const { return d->modalities; }
void ChatCompletionRequest::setModalities(const QStringList &modalities)
{
    d->modalities = modalities;
}

QJsonObject ChatCompletionRequest::prediction() const { return d->prediction; }
void ChatCompletionRequest::setPrediction(const QJsonObject &prediction)
{
    d->prediction = prediction;
}

std::optional<bool> ChatCompletionRequest::parallelToolCalls() const
{
    return d->parallelToolCalls;
}
void ChatCompletionRequest::setParallelToolCalls(bool parallelToolCalls)
{
    d->parallelToolCalls = parallelToolCalls;
}

std::optional<int> ChatCompletionRequest::maxTokens() const { return d->maxTokens; }
void ChatCompletionRequest::setMaxTokens(int tokens) { d->maxTokens = tokens; }

QString ChatCompletionRequest::serviceTier() const { return d->serviceTier; }
void ChatCompletionRequest::setServiceTier(const QString &serviceTier)
{
    d->serviceTier = serviceTier;
}

std::optional<bool> ChatCompletionRequest::store() const { return d->store; }
void ChatCompletionRequest::setStore(bool store) { d->store = store; }

QJsonObject ChatCompletionRequest::metadata() const { return d->metadata; }
void ChatCompletionRequest::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

QString ChatCompletionRequest::user() const { return d->user; }
void ChatCompletionRequest::setUser(const QString &user) { d->user = user; }

QString ChatCompletionRequest::safetyIdentifier() const { return d->safetyIdentifier; }
void ChatCompletionRequest::setSafetyIdentifier(const QString &safetyIdentifier)
{
    d->safetyIdentifier = safetyIdentifier;
}

QString ChatCompletionRequest::promptCacheKey() const { return d->promptCacheKey; }
void ChatCompletionRequest::setPromptCacheKey(const QString &promptCacheKey)
{
    d->promptCacheKey = promptCacheKey;
}

QString ChatCompletionRequest::reasoningEffort() const { return d->reasoningEffort; }
void ChatCompletionRequest::setReasoningEffort(const QString &effort)
{
    d->reasoningEffort = effort;
}

QJsonObject ChatCompletionRequest::webSearchOptions() const { return d->webSearchOptions; }
void ChatCompletionRequest::setWebSearchOptions(const QJsonObject &options)
{
    d->webSearchOptions = options;
}

QJsonObject ChatCompletionRequest::extraBody() const { return d->extraBody; }
void ChatCompletionRequest::setExtraBody(const QJsonObject &extra) { d->extraBody = extra; }

QJsonObject ChatCompletionRequest::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("model"), d->model);

    QJsonArray messages;
    for (const Message &message : d->messages)
        messages.append(message.toJson());
    json.insert(QStringLiteral("messages"), messages);

    if (!d->tools.isEmpty()) {
        QJsonArray tools;
        for (const Tool &tool : d->tools)
            tools.append(tool.toJson());
        json.insert(QStringLiteral("tools"), tools);
    }

    if (d->toolChoice)
        json.insert(QStringLiteral("tool_choice"), *d->toolChoice);
    if (d->temperature)
        json.insert(QStringLiteral("temperature"), *d->temperature);
    if (d->topP)
        json.insert(QStringLiteral("top_p"), *d->topP);
    if (d->maxCompletionTokens)
        json.insert(QStringLiteral("max_completion_tokens"), *d->maxCompletionTokens);
    if (d->n)
        json.insert(QStringLiteral("n"), *d->n);
    if (d->stream)
        json.insert(QStringLiteral("stream"), *d->stream);
    if (d->frequencyPenalty)
        json.insert(QStringLiteral("frequency_penalty"), *d->frequencyPenalty);
    if (d->presencePenalty)
        json.insert(QStringLiteral("presence_penalty"), *d->presencePenalty);
    if (!d->logitBias.isEmpty())
        json.insert(QStringLiteral("logit_bias"), d->logitBias);
    if (d->seed)
        json.insert(QStringLiteral("seed"), *d->seed);
    if (d->stop)
        json.insert(QStringLiteral("stop"), *d->stop);
    if (d->logprobs)
        json.insert(QStringLiteral("logprobs"), *d->logprobs);
    if (d->topLogprobs)
        json.insert(QStringLiteral("top_logprobs"), *d->topLogprobs);
    if (!d->streamOptions.isEmpty())
        json.insert(QStringLiteral("stream_options"), d->streamOptions);
    if (!d->modalities.isEmpty())
        json.insert(QStringLiteral("modalities"), QJsonArray::fromStringList(d->modalities));
    if (!d->prediction.isEmpty())
        json.insert(QStringLiteral("prediction"), d->prediction);
    if (d->parallelToolCalls)
        json.insert(QStringLiteral("parallel_tool_calls"), *d->parallelToolCalls);
    if (d->maxTokens)
        json.insert(QStringLiteral("max_tokens"), *d->maxTokens);
    detail::insertIfNotEmpty(json, QStringLiteral("service_tier"), d->serviceTier);
    if (d->store)
        json.insert(QStringLiteral("store"), *d->store);
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    detail::insertIfNotEmpty(json, QStringLiteral("user"), d->user);
    detail::insertIfNotEmpty(json, QStringLiteral("safety_identifier"), d->safetyIdentifier);
    detail::insertIfNotEmpty(json, QStringLiteral("prompt_cache_key"), d->promptCacheKey);
    detail::insertIfNotEmpty(json, QStringLiteral("reasoning_effort"), d->reasoningEffort);
    if (!d->webSearchOptions.isEmpty())
        json.insert(QStringLiteral("web_search_options"), d->webSearchOptions);

    // Merge any provider-specific extra fields last (without overriding core).
    for (auto it = d->extraBody.constBegin(); it != d->extraBody.constEnd(); ++it) {
        if (!json.contains(it.key()))
            json.insert(it.key(), it.value());
    }
    return json;
}

ChatCompletionRequest ChatCompletionRequest::fromJson(const QJsonObject &json)
{
    ChatCompletionRequest request;
    request.d->model = detail::stringOr(json, QStringLiteral("model"));

    const QJsonArray messages = json.value(QStringLiteral("messages")).toArray();
    for (const QJsonValue &value : messages)
        request.d->messages.append(Message::fromJson(value.toObject()));

    const QJsonArray tools = json.value(QStringLiteral("tools")).toArray();
    for (const QJsonValue &value : tools)
        request.d->tools.append(Tool::fromJson(value.toObject()));

    if (json.contains(QStringLiteral("tool_choice")))
        request.d->toolChoice = json.value(QStringLiteral("tool_choice"));
    if (json.contains(QStringLiteral("temperature")))
        request.d->temperature = json.value(QStringLiteral("temperature")).toDouble();
    if (json.contains(QStringLiteral("top_p")))
        request.d->topP = json.value(QStringLiteral("top_p")).toDouble();
    if (json.contains(QStringLiteral("max_completion_tokens")))
        request.d->maxCompletionTokens
                = json.value(QStringLiteral("max_completion_tokens")).toInt();
    if (json.contains(QStringLiteral("n")))
        request.d->n = json.value(QStringLiteral("n")).toInt();
    if (json.contains(QStringLiteral("stream")))
        request.d->stream = json.value(QStringLiteral("stream")).toBool();
    if (json.contains(QStringLiteral("frequency_penalty")))
        request.d->frequencyPenalty = json.value(QStringLiteral("frequency_penalty")).toDouble();
    if (json.contains(QStringLiteral("presence_penalty")))
        request.d->presencePenalty = json.value(QStringLiteral("presence_penalty")).toDouble();
    request.d->logitBias = json.value(QStringLiteral("logit_bias")).toObject();
    if (json.contains(QStringLiteral("seed")))
        request.d->seed = json.value(QStringLiteral("seed")).toInt();
    if (json.contains(QStringLiteral("stop")))
        request.d->stop = json.value(QStringLiteral("stop"));
    if (json.contains(QStringLiteral("logprobs")))
        request.d->logprobs = json.value(QStringLiteral("logprobs")).toBool();
    if (json.contains(QStringLiteral("top_logprobs")))
        request.d->topLogprobs = json.value(QStringLiteral("top_logprobs")).toInt();
    request.d->streamOptions = json.value(QStringLiteral("stream_options")).toObject();
    const QJsonArray modalities = json.value(QStringLiteral("modalities")).toArray();
    for (const QJsonValue &value : modalities)
        request.d->modalities.append(value.toString());
    request.d->prediction = json.value(QStringLiteral("prediction")).toObject();
    if (json.contains(QStringLiteral("parallel_tool_calls")))
        request.d->parallelToolCalls = json.value(QStringLiteral("parallel_tool_calls")).toBool();
    if (json.contains(QStringLiteral("max_tokens")))
        request.d->maxTokens = json.value(QStringLiteral("max_tokens")).toInt();
    request.d->serviceTier = detail::stringOr(json, QStringLiteral("service_tier"));
    if (json.contains(QStringLiteral("store")))
        request.d->store = json.value(QStringLiteral("store")).toBool();
    request.d->metadata = json.value(QStringLiteral("metadata")).toObject();
    request.d->user = detail::stringOr(json, QStringLiteral("user"));
    request.d->safetyIdentifier = detail::stringOr(json, QStringLiteral("safety_identifier"));
    request.d->promptCacheKey = detail::stringOr(json, QStringLiteral("prompt_cache_key"));
    request.d->reasoningEffort = detail::stringOr(json, QStringLiteral("reasoning_effort"));
    request.d->webSearchOptions = json.value(QStringLiteral("web_search_options")).toObject();

    return request;
}

bool ChatCompletionRequest::operator==(const ChatCompletionRequest &other) const
{
    return d->model == other.d->model && d->messages == other.d->messages
           && d->tools == other.d->tools && d->toolChoice == other.d->toolChoice
           && d->temperature == other.d->temperature && d->topP == other.d->topP
           && d->maxCompletionTokens == other.d->maxCompletionTokens && d->n == other.d->n
           && d->stream == other.d->stream && d->frequencyPenalty == other.d->frequencyPenalty
           && d->presencePenalty == other.d->presencePenalty && d->logitBias == other.d->logitBias
           && d->seed == other.d->seed && d->stop == other.d->stop
           && d->logprobs == other.d->logprobs && d->topLogprobs == other.d->topLogprobs
           && d->streamOptions == other.d->streamOptions && d->modalities == other.d->modalities
           && d->prediction == other.d->prediction
           && d->parallelToolCalls == other.d->parallelToolCalls
           && d->maxTokens == other.d->maxTokens && d->serviceTier == other.d->serviceTier
           && d->store == other.d->store && d->metadata == other.d->metadata
           && d->user == other.d->user && d->safetyIdentifier == other.d->safetyIdentifier
           && d->promptCacheKey == other.d->promptCacheKey
           && d->reasoningEffort == other.d->reasoningEffort
           && d->webSearchOptions == other.d->webSearchOptions
           && d->extraBody == other.d->extraBody;
}

} // namespace Core
} // namespace QtOpenAi
