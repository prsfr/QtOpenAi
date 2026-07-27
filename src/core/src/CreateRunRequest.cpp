// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/CreateRunRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class CreateRunRequestData : public QSharedData
{
public:
    QString assistantId;
    QString model;
    QString instructions;
    QString additionalInstructions;
    QList<ThreadMessageInput> additionalMessages;
    QJsonArray tools;
    QJsonObject metadata;
    std::optional<double> temperature;
    std::optional<double> topP;
    std::optional<int> maxPromptTokens;
    std::optional<int> maxCompletionTokens;
    QJsonObject truncationStrategy;
    QJsonValue toolChoice = QJsonValue::Undefined;
    std::optional<bool> parallelToolCalls;
    QJsonValue responseFormat = QJsonValue::Undefined;
    CreateThreadRequest thread;
    std::optional<bool> stream;
};

CreateRunRequest::CreateRunRequest()
    : d(new CreateRunRequestData)
{ }

CreateRunRequest::CreateRunRequest(QString assistantId)
    : d(new CreateRunRequestData)
{
    d->assistantId = std::move(assistantId);
}

CreateRunRequest::CreateRunRequest(const CreateRunRequest &other) = default;
CreateRunRequest::CreateRunRequest(CreateRunRequest &&other) noexcept = default;
CreateRunRequest &CreateRunRequest::operator=(const CreateRunRequest &other) = default;
CreateRunRequest &CreateRunRequest::operator=(CreateRunRequest &&other) noexcept = default;
CreateRunRequest::~CreateRunRequest() = default;

QString CreateRunRequest::assistantId() const { return d->assistantId; }
void CreateRunRequest::setAssistantId(const QString &assistantId) { d->assistantId = assistantId; }

QString CreateRunRequest::model() const { return d->model; }
void CreateRunRequest::setModel(const QString &model) { d->model = model; }

QString CreateRunRequest::instructions() const { return d->instructions; }
void CreateRunRequest::setInstructions(const QString &instructions)
{
    d->instructions = instructions;
}

QString CreateRunRequest::additionalInstructions() const { return d->additionalInstructions; }
void CreateRunRequest::setAdditionalInstructions(const QString &additionalInstructions)
{
    d->additionalInstructions = additionalInstructions;
}

QList<ThreadMessageInput> CreateRunRequest::additionalMessages() const
{
    return d->additionalMessages;
}

void CreateRunRequest::setAdditionalMessages(const QList<ThreadMessageInput> &messages)
{
    d->additionalMessages = messages;
}

void CreateRunRequest::addMessage(const ThreadMessageInput &message)
{
    d->additionalMessages.append(message);
}

void CreateRunRequest::addUserMessage(const QString &text)
{
    ThreadMessageInput message;
    message.role = Role::User;
    message.text = text;
    d->additionalMessages.append(message);
}

QJsonArray CreateRunRequest::tools() const { return d->tools; }
void CreateRunRequest::setTools(const QJsonArray &tools) { d->tools = tools; }

void CreateRunRequest::addTool(const Tool &tool) { d->tools.append(tool.toJson()); }
void CreateRunRequest::addTool(const QJsonObject &tool) { d->tools.append(tool); }

QJsonObject CreateRunRequest::metadata() const { return d->metadata; }
void CreateRunRequest::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

std::optional<double> CreateRunRequest::temperature() const { return d->temperature; }
void CreateRunRequest::setTemperature(double temperature) { d->temperature = temperature; }

std::optional<double> CreateRunRequest::topP() const { return d->topP; }
void CreateRunRequest::setTopP(double topP) { d->topP = topP; }

std::optional<int> CreateRunRequest::maxPromptTokens() const { return d->maxPromptTokens; }
void CreateRunRequest::setMaxPromptTokens(int maxPromptTokens)
{
    d->maxPromptTokens = maxPromptTokens;
}

std::optional<int> CreateRunRequest::maxCompletionTokens() const { return d->maxCompletionTokens; }
void CreateRunRequest::setMaxCompletionTokens(int maxCompletionTokens)
{
    d->maxCompletionTokens = maxCompletionTokens;
}

QJsonObject CreateRunRequest::truncationStrategy() const { return d->truncationStrategy; }
void CreateRunRequest::setTruncationStrategy(const QJsonObject &truncationStrategy)
{
    d->truncationStrategy = truncationStrategy;
}

QJsonValue CreateRunRequest::toolChoice() const { return d->toolChoice; }
void CreateRunRequest::setToolChoice(const QJsonValue &toolChoice) { d->toolChoice = toolChoice; }

std::optional<bool> CreateRunRequest::parallelToolCalls() const { return d->parallelToolCalls; }
void CreateRunRequest::setParallelToolCalls(bool parallelToolCalls)
{
    d->parallelToolCalls = parallelToolCalls;
}

QJsonValue CreateRunRequest::responseFormat() const { return d->responseFormat; }
void CreateRunRequest::setResponseFormat(const QJsonValue &responseFormat)
{
    d->responseFormat = responseFormat;
}

void CreateRunRequest::setResponseFormat(const ResponseFormat &responseFormat)
{
    d->responseFormat = responseFormat.toJson();
}

CreateThreadRequest CreateRunRequest::thread() const { return d->thread; }
void CreateRunRequest::setThread(const CreateThreadRequest &thread) { d->thread = thread; }

std::optional<bool> CreateRunRequest::stream() const { return d->stream; }
void CreateRunRequest::setStream(bool stream) { d->stream = stream; }

QJsonObject CreateRunRequest::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("assistant_id"), d->assistantId);
    detail::insertIfNotEmpty(json, QStringLiteral("model"), d->model);
    detail::insertIfNotEmpty(json, QStringLiteral("instructions"), d->instructions);
    detail::insertIfNotEmpty(json, QStringLiteral("additional_instructions"),
                             d->additionalInstructions);
    if (!d->additionalMessages.isEmpty()) {
        QJsonArray messages;
        for (const ThreadMessageInput &message : d->additionalMessages)
            messages.append(message.toJson());
        json.insert(QStringLiteral("additional_messages"), messages);
    }
    if (!d->tools.isEmpty())
        json.insert(QStringLiteral("tools"), d->tools);
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    detail::insertIfSet(json, QStringLiteral("temperature"), d->temperature);
    detail::insertIfSet(json, QStringLiteral("top_p"), d->topP);
    detail::insertIfSet(json, QStringLiteral("max_prompt_tokens"), d->maxPromptTokens);
    detail::insertIfSet(json, QStringLiteral("max_completion_tokens"), d->maxCompletionTokens);
    if (!d->truncationStrategy.isEmpty())
        json.insert(QStringLiteral("truncation_strategy"), d->truncationStrategy);
    if (!d->toolChoice.isUndefined())
        json.insert(QStringLiteral("tool_choice"), d->toolChoice);
    detail::insertIfSet(json, QStringLiteral("parallel_tool_calls"), d->parallelToolCalls);
    if (!d->responseFormat.isUndefined())
        json.insert(QStringLiteral("response_format"), d->responseFormat);
    if (!d->thread.isEmpty())
        json.insert(QStringLiteral("thread"), d->thread.toJson());
    detail::insertIfSet(json, QStringLiteral("stream"), d->stream);
    return json;
}

bool CreateRunRequest::operator==(const CreateRunRequest &other) const
{
    return d->assistantId == other.d->assistantId && d->model == other.d->model
           && d->instructions == other.d->instructions
           && d->additionalInstructions == other.d->additionalInstructions
           && d->additionalMessages == other.d->additionalMessages && d->tools == other.d->tools
           && d->metadata == other.d->metadata && d->temperature == other.d->temperature
           && d->topP == other.d->topP && d->maxPromptTokens == other.d->maxPromptTokens
           && d->maxCompletionTokens == other.d->maxCompletionTokens
           && d->truncationStrategy == other.d->truncationStrategy
           && d->toolChoice == other.d->toolChoice
           && d->parallelToolCalls == other.d->parallelToolCalls
           && d->responseFormat == other.d->responseFormat && d->thread == other.d->thread
           && d->stream == other.d->stream;
}

} // namespace Core
} // namespace QtOpenAi
