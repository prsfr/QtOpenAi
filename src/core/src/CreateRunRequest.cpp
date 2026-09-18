// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/CreateRunRequest.h"

#include "JsonHelpers_p.h"
#include "RunFields_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

// The thirteen shared fields come from the private base; these four are
// genuinely create-only. See RunFields_p.h.
class CreateRunRequestData : public detail::RunFieldsData
{
public:
    QString additionalInstructions;
    QList<ThreadMessageInput> additionalMessages;
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
    detail::insertRunFields(json, *d);
    detail::insertIfNotEmpty(json, QStringLiteral("additional_instructions"),
                             d->additionalInstructions);
    if (!d->additionalMessages.isEmpty()) {
        QJsonArray messages;
        for (const ThreadMessageInput &message : d->additionalMessages)
            messages.append(message.toJson());
        json.insert(QStringLiteral("additional_messages"), messages);
    }
    if (!d->thread.isEmpty())
        json.insert(QStringLiteral("thread"), d->thread.toJson());
    detail::insertIfSet(json, QStringLiteral("stream"), d->stream);
    return json;
}

bool CreateRunRequest::operator==(const CreateRunRequest &other) const
{
    return d->additionalInstructions == other.d->additionalInstructions
           && d->additionalMessages == other.d->additionalMessages && d->thread == other.d->thread
           && d->stream == other.d->stream && detail::runFieldsEqual(*d, *other.d);
}

} // namespace Core
} // namespace QtOpenAi
