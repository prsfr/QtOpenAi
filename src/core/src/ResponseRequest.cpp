// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ResponseRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ResponseRequestData : public QSharedData
{
public:
    QString model;
    QJsonValue input;
    QString instructions;
    QList<Tool> tools;
    std::optional<QJsonValue> toolChoice;
    std::optional<ResponseFormat> textFormat;
    std::optional<int> maxOutputTokens;
    std::optional<double> temperature;
    std::optional<double> topP;
    std::optional<bool> store;
    QString previousResponseId;
    QString reasoningEffort;
    QJsonObject metadata;
    std::optional<bool> stream;
    QJsonObject extraBody;
};

ResponseRequest::ResponseRequest()
    : d(new ResponseRequestData)
{ }

ResponseRequest::ResponseRequest(QString model, QString input)
    : d(new ResponseRequestData)
{
    d->model = std::move(model);
    d->input = std::move(input);
}

ResponseRequest::ResponseRequest(const ResponseRequest &other) = default;
ResponseRequest::ResponseRequest(ResponseRequest &&other) noexcept = default;
ResponseRequest &ResponseRequest::operator=(const ResponseRequest &other) = default;
ResponseRequest &ResponseRequest::operator=(ResponseRequest &&other) noexcept = default;
ResponseRequest::~ResponseRequest() = default;

QString ResponseRequest::model() const { return d->model; }
void ResponseRequest::setModel(const QString &model) { d->model = model; }

QJsonValue ResponseRequest::input() const { return d->input; }
void ResponseRequest::setInput(const QJsonValue &input) { d->input = input; }
void ResponseRequest::setInput(const QString &input) { d->input = input; }

QString ResponseRequest::instructions() const { return d->instructions; }
void ResponseRequest::setInstructions(const QString &instructions)
{
    d->instructions = instructions;
}

QList<Tool> ResponseRequest::tools() const { return d->tools; }
void ResponseRequest::setTools(const QList<Tool> &tools) { d->tools = tools; }
void ResponseRequest::addTool(const Tool &tool) { d->tools.append(tool); }

std::optional<QJsonValue> ResponseRequest::toolChoice() const { return d->toolChoice; }
void ResponseRequest::setToolChoice(const QJsonValue &toolChoice) { d->toolChoice = toolChoice; }

std::optional<ResponseFormat> ResponseRequest::textFormat() const { return d->textFormat; }
void ResponseRequest::setTextFormat(const ResponseFormat &format) { d->textFormat = format; }

std::optional<int> ResponseRequest::maxOutputTokens() const { return d->maxOutputTokens; }
void ResponseRequest::setMaxOutputTokens(int tokens) { d->maxOutputTokens = tokens; }

std::optional<double> ResponseRequest::temperature() const { return d->temperature; }
void ResponseRequest::setTemperature(double temperature) { d->temperature = temperature; }

std::optional<double> ResponseRequest::topP() const { return d->topP; }
void ResponseRequest::setTopP(double topP) { d->topP = topP; }

std::optional<bool> ResponseRequest::store() const { return d->store; }
void ResponseRequest::setStore(bool store) { d->store = store; }

QString ResponseRequest::previousResponseId() const { return d->previousResponseId; }
void ResponseRequest::setPreviousResponseId(const QString &id) { d->previousResponseId = id; }

QString ResponseRequest::reasoningEffort() const { return d->reasoningEffort; }
void ResponseRequest::setReasoningEffort(const QString &effort) { d->reasoningEffort = effort; }

QJsonObject ResponseRequest::metadata() const { return d->metadata; }
void ResponseRequest::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

std::optional<bool> ResponseRequest::stream() const { return d->stream; }
void ResponseRequest::setStream(bool stream) { d->stream = stream; }

QJsonObject ResponseRequest::extraBody() const { return d->extraBody; }
void ResponseRequest::setExtraBody(const QJsonObject &extra) { d->extraBody = extra; }

QJsonObject ResponseRequest::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("model"), d->model);
    if (!d->input.isNull() && !d->input.isUndefined())
        json.insert(QStringLiteral("input"), d->input);
    detail::insertIfNotEmpty(json, QStringLiteral("instructions"), d->instructions);

    if (!d->tools.isEmpty()) {
        QJsonArray tools;
        for (const Tool &tool : d->tools)
            tools.append(tool.toJson());
        json.insert(QStringLiteral("tools"), tools);
    }

    if (d->toolChoice)
        json.insert(QStringLiteral("tool_choice"), *d->toolChoice);
    if (d->textFormat) {
        QJsonObject text;
        text.insert(QStringLiteral("format"), d->textFormat->toTextFormatJson());
        json.insert(QStringLiteral("text"), text);
    }
    if (d->maxOutputTokens)
        json.insert(QStringLiteral("max_output_tokens"), *d->maxOutputTokens);
    if (d->temperature)
        json.insert(QStringLiteral("temperature"), *d->temperature);
    if (d->topP)
        json.insert(QStringLiteral("top_p"), *d->topP);
    if (d->store)
        json.insert(QStringLiteral("store"), *d->store);
    detail::insertIfNotEmpty(json, QStringLiteral("previous_response_id"), d->previousResponseId);
    if (!d->reasoningEffort.isEmpty()) {
        QJsonObject reasoning;
        reasoning.insert(QStringLiteral("effort"), d->reasoningEffort);
        json.insert(QStringLiteral("reasoning"), reasoning);
    }
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    if (d->stream)
        json.insert(QStringLiteral("stream"), *d->stream);

    // Merge any provider-specific extra fields last (without overriding core).
    for (auto it = d->extraBody.constBegin(); it != d->extraBody.constEnd(); ++it) {
        if (!json.contains(it.key()))
            json.insert(it.key(), it.value());
    }
    return json;
}

ResponseRequest ResponseRequest::fromJson(const QJsonObject &json)
{
    ResponseRequest request;
    request.d->model = detail::stringOr(json, QStringLiteral("model"));
    request.d->input = json.value(QStringLiteral("input"));
    request.d->instructions = detail::stringOr(json, QStringLiteral("instructions"));

    const QJsonArray tools = json.value(QStringLiteral("tools")).toArray();
    for (const QJsonValue &value : tools)
        request.d->tools.append(Tool::fromJson(value.toObject()));

    if (json.contains(QStringLiteral("tool_choice")))
        request.d->toolChoice = json.value(QStringLiteral("tool_choice"));
    const QJsonObject text = json.value(QStringLiteral("text")).toObject();
    if (text.contains(QStringLiteral("format")))
        request.d->textFormat
                = ResponseFormat::fromJson(text.value(QStringLiteral("format")).toObject());
    if (json.contains(QStringLiteral("max_output_tokens")))
        request.d->maxOutputTokens = json.value(QStringLiteral("max_output_tokens")).toInt();
    if (json.contains(QStringLiteral("temperature")))
        request.d->temperature = json.value(QStringLiteral("temperature")).toDouble();
    if (json.contains(QStringLiteral("top_p")))
        request.d->topP = json.value(QStringLiteral("top_p")).toDouble();
    if (json.contains(QStringLiteral("store")))
        request.d->store = json.value(QStringLiteral("store")).toBool();
    request.d->previousResponseId = detail::stringOr(json, QStringLiteral("previous_response_id"));
    const QJsonObject reasoning = json.value(QStringLiteral("reasoning")).toObject();
    request.d->reasoningEffort = reasoning.value(QStringLiteral("effort")).toString();
    request.d->metadata = json.value(QStringLiteral("metadata")).toObject();
    if (json.contains(QStringLiteral("stream")))
        request.d->stream = json.value(QStringLiteral("stream")).toBool();

    return request;
}

bool ResponseRequest::operator==(const ResponseRequest &other) const
{
    return d->model == other.d->model && d->input == other.d->input
           && d->instructions == other.d->instructions && d->tools == other.d->tools
           && d->toolChoice == other.d->toolChoice && d->textFormat == other.d->textFormat
           && d->maxOutputTokens == other.d->maxOutputTokens
           && d->temperature == other.d->temperature && d->topP == other.d->topP
           && d->store == other.d->store && d->previousResponseId == other.d->previousResponseId
           && d->reasoningEffort == other.d->reasoningEffort && d->metadata == other.d->metadata
           && d->stream == other.d->stream && d->extraBody == other.d->extraBody;
}

} // namespace Core
} // namespace QtOpenAi
