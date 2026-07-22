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
    QJsonObject extraBody;
};

ChatCompletionRequest::ChatCompletionRequest()
    : d(new ChatCompletionRequestData)
{
}

ChatCompletionRequest::ChatCompletionRequest(QString model, QList<Message> messages)
    : d(new ChatCompletionRequestData)
{
    d->model = std::move(model);
    d->messages = std::move(messages);
}

ChatCompletionRequest::ChatCompletionRequest(const ChatCompletionRequest &other) = default;
ChatCompletionRequest::ChatCompletionRequest(ChatCompletionRequest &&other) noexcept = default;
ChatCompletionRequest &ChatCompletionRequest::operator=(const ChatCompletionRequest &other) = default;
ChatCompletionRequest &ChatCompletionRequest::operator=(ChatCompletionRequest &&other) noexcept = default;
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
void ChatCompletionRequest::setToolChoice(const QJsonValue &toolChoice) { d->toolChoice = toolChoice; }

std::optional<double> ChatCompletionRequest::temperature() const { return d->temperature; }
void ChatCompletionRequest::setTemperature(double temperature) { d->temperature = temperature; }

std::optional<double> ChatCompletionRequest::topP() const { return d->topP; }
void ChatCompletionRequest::setTopP(double topP) { d->topP = topP; }

std::optional<int> ChatCompletionRequest::maxCompletionTokens() const { return d->maxCompletionTokens; }
void ChatCompletionRequest::setMaxCompletionTokens(int tokens) { d->maxCompletionTokens = tokens; }

std::optional<int> ChatCompletionRequest::n() const { return d->n; }
void ChatCompletionRequest::setN(int n) { d->n = n; }

std::optional<bool> ChatCompletionRequest::stream() const { return d->stream; }
void ChatCompletionRequest::setStream(bool stream) { d->stream = stream; }

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
        request.d->maxCompletionTokens = json.value(QStringLiteral("max_completion_tokens")).toInt();
    if (json.contains(QStringLiteral("n")))
        request.d->n = json.value(QStringLiteral("n")).toInt();
    if (json.contains(QStringLiteral("stream")))
        request.d->stream = json.value(QStringLiteral("stream")).toBool();

    return request;
}

bool ChatCompletionRequest::operator==(const ChatCompletionRequest &other) const
{
    return d->model == other.d->model
        && d->messages == other.d->messages
        && d->tools == other.d->tools
        && d->toolChoice == other.d->toolChoice
        && d->temperature == other.d->temperature
        && d->topP == other.d->topP
        && d->maxCompletionTokens == other.d->maxCompletionTokens
        && d->n == other.d->n
        && d->stream == other.d->stream
        && d->extraBody == other.d->extraBody;
}

} // namespace Core
} // namespace QtOpenAi
