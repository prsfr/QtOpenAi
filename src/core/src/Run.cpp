// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Run.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

// --- ToolOutput ------------------------------------------------------------

QJsonObject ToolOutput::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("tool_call_id"), toolCallId);
    json.insert(QStringLiteral("output"), output);
    return json;
}

ToolOutput ToolOutput::fromJson(const QJsonObject &json)
{
    ToolOutput output;
    output.toolCallId = detail::stringOr(json, QStringLiteral("tool_call_id"));
    output.output = detail::stringOr(json, QStringLiteral("output"));
    return output;
}

// --- Run -------------------------------------------------------------------

class RunData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    QString threadId;
    QString assistantId;
    RunStatus status = RunStatus::Queued;
    QString requiredActionType;
    QList<ToolCall> requiredToolCalls;
    QString errorCode;
    QString errorMessage;
    QJsonObject incompleteDetails;
    qint64 expiresAt = 0;
    qint64 startedAt = 0;
    qint64 cancelledAt = 0;
    qint64 failedAt = 0;
    qint64 completedAt = 0;
    QString model;
    QString instructions;
    QJsonArray tools;
    QJsonObject metadata;
    Usage usage;
    std::optional<double> temperature;
    std::optional<double> topP;
    std::optional<int> maxPromptTokens;
    std::optional<int> maxCompletionTokens;
    QJsonObject truncationStrategy;
    QJsonValue toolChoice = QJsonValue::Undefined;
    std::optional<bool> parallelToolCalls;
    QJsonValue responseFormat = QJsonValue::Undefined;
};

Run::Run()
    : d(new RunData)
{ }

Run::Run(const Run &other) = default;
Run::Run(Run &&other) noexcept = default;
Run &Run::operator=(const Run &other) = default;
Run &Run::operator=(Run &&other) noexcept = default;
Run::~Run() = default;

QString Run::id() const { return d->id; }
void Run::setId(const QString &id) { d->id = id; }

QString Run::object() const { return d->object; }
void Run::setObject(const QString &object) { d->object = object; }

qint64 Run::createdAt() const { return d->createdAt; }
void Run::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString Run::threadId() const { return d->threadId; }
void Run::setThreadId(const QString &threadId) { d->threadId = threadId; }

QString Run::assistantId() const { return d->assistantId; }
void Run::setAssistantId(const QString &assistantId) { d->assistantId = assistantId; }

RunStatus Run::status() const { return d->status; }
void Run::setStatus(RunStatus status) { d->status = status; }

QString Run::requiredActionType() const { return d->requiredActionType; }
void Run::setRequiredActionType(const QString &requiredActionType)
{
    d->requiredActionType = requiredActionType;
}

QList<ToolCall> Run::requiredToolCalls() const { return d->requiredToolCalls; }
void Run::setRequiredToolCalls(const QList<ToolCall> &toolCalls)
{
    d->requiredToolCalls = toolCalls;
}

QString Run::errorCode() const { return d->errorCode; }
void Run::setErrorCode(const QString &errorCode) { d->errorCode = errorCode; }

QString Run::errorMessage() const { return d->errorMessage; }
void Run::setErrorMessage(const QString &errorMessage) { d->errorMessage = errorMessage; }

QJsonObject Run::incompleteDetails() const { return d->incompleteDetails; }
void Run::setIncompleteDetails(const QJsonObject &incompleteDetails)
{
    d->incompleteDetails = incompleteDetails;
}

qint64 Run::expiresAt() const { return d->expiresAt; }
void Run::setExpiresAt(qint64 expiresAt) { d->expiresAt = expiresAt; }

qint64 Run::startedAt() const { return d->startedAt; }
void Run::setStartedAt(qint64 startedAt) { d->startedAt = startedAt; }

qint64 Run::cancelledAt() const { return d->cancelledAt; }
void Run::setCancelledAt(qint64 cancelledAt) { d->cancelledAt = cancelledAt; }

qint64 Run::failedAt() const { return d->failedAt; }
void Run::setFailedAt(qint64 failedAt) { d->failedAt = failedAt; }

qint64 Run::completedAt() const { return d->completedAt; }
void Run::setCompletedAt(qint64 completedAt) { d->completedAt = completedAt; }

QString Run::model() const { return d->model; }
void Run::setModel(const QString &model) { d->model = model; }

QString Run::instructions() const { return d->instructions; }
void Run::setInstructions(const QString &instructions) { d->instructions = instructions; }

QJsonArray Run::tools() const { return d->tools; }
void Run::setTools(const QJsonArray &tools) { d->tools = tools; }

QJsonObject Run::metadata() const { return d->metadata; }
void Run::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

Usage Run::usage() const { return d->usage; }
void Run::setUsage(const Usage &usage) { d->usage = usage; }

std::optional<double> Run::temperature() const { return d->temperature; }
void Run::setTemperature(double temperature) { d->temperature = temperature; }

std::optional<double> Run::topP() const { return d->topP; }
void Run::setTopP(double topP) { d->topP = topP; }

std::optional<int> Run::maxPromptTokens() const { return d->maxPromptTokens; }
void Run::setMaxPromptTokens(int maxPromptTokens) { d->maxPromptTokens = maxPromptTokens; }

std::optional<int> Run::maxCompletionTokens() const { return d->maxCompletionTokens; }
void Run::setMaxCompletionTokens(int maxCompletionTokens)
{
    d->maxCompletionTokens = maxCompletionTokens;
}

QJsonObject Run::truncationStrategy() const { return d->truncationStrategy; }
void Run::setTruncationStrategy(const QJsonObject &truncationStrategy)
{
    d->truncationStrategy = truncationStrategy;
}

QJsonValue Run::toolChoice() const { return d->toolChoice; }
void Run::setToolChoice(const QJsonValue &toolChoice) { d->toolChoice = toolChoice; }

std::optional<bool> Run::parallelToolCalls() const { return d->parallelToolCalls; }
void Run::setParallelToolCalls(bool parallelToolCalls) { d->parallelToolCalls = parallelToolCalls; }

QJsonValue Run::responseFormat() const { return d->responseFormat; }
void Run::setResponseFormat(const QJsonValue &responseFormat)
{
    d->responseFormat = responseFormat;
}

bool Run::isTerminal() const { return Core::isTerminal(d->status); }

bool Run::requiresAction() const { return d->status == RunStatus::RequiresAction; }

QJsonObject Run::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("thread_id"), d->threadId);
    detail::insertIfNotEmpty(json, QStringLiteral("assistant_id"), d->assistantId);
    json.insert(QStringLiteral("status"), runStatusToString(d->status));
    // Keyed on the calls as well as the type, so a run assembled through the
    // setters cannot lose them to an unset sibling field.
    if (!d->requiredActionType.isEmpty() || !d->requiredToolCalls.isEmpty()) {
        QJsonArray calls;
        for (const ToolCall &call : d->requiredToolCalls)
            calls.append(call.toJson());
        json.insert(QStringLiteral("required_action"),
                    QJsonObject {
                            {QStringLiteral("type"), d->requiredActionType},
                            {QStringLiteral("submit_tool_outputs"),
                             QJsonObject {{QStringLiteral("tool_calls"), calls}}},
                    });
    }
    if (!d->errorCode.isEmpty() || !d->errorMessage.isEmpty()) {
        QJsonObject error;
        detail::insertIfNotEmpty(error, QStringLiteral("code"), d->errorCode);
        detail::insertIfNotEmpty(error, QStringLiteral("message"), d->errorMessage);
        json.insert(QStringLiteral("last_error"), error);
    }
    if (!d->incompleteDetails.isEmpty())
        json.insert(QStringLiteral("incomplete_details"), d->incompleteDetails);
    detail::insertIfNonZero(json, QStringLiteral("expires_at"), d->expiresAt);
    detail::insertIfNonZero(json, QStringLiteral("started_at"), d->startedAt);
    detail::insertIfNonZero(json, QStringLiteral("cancelled_at"), d->cancelledAt);
    detail::insertIfNonZero(json, QStringLiteral("failed_at"), d->failedAt);
    detail::insertIfNonZero(json, QStringLiteral("completed_at"), d->completedAt);
    detail::insertIfNotEmpty(json, QStringLiteral("model"), d->model);
    detail::insertIfNotEmpty(json, QStringLiteral("instructions"), d->instructions);
    if (!d->tools.isEmpty())
        json.insert(QStringLiteral("tools"), d->tools);
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    json.insert(QStringLiteral("usage"), d->usage.toJson());
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
    return json;
}

Run Run::fromJson(const QJsonObject &json)
{
    Run run;
    run.d->id = detail::stringOr(json, QStringLiteral("id"));
    run.d->object = detail::stringOr(json, QStringLiteral("object"));
    run.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    run.d->threadId = detail::stringOr(json, QStringLiteral("thread_id"));
    run.d->assistantId = detail::stringOr(json, QStringLiteral("assistant_id"));
    run.d->status = runStatusFromString(detail::stringOr(json, QStringLiteral("status")));

    const QJsonObject requiredAction = json.value(QStringLiteral("required_action")).toObject();
    run.d->requiredActionType = detail::stringOr(requiredAction, QStringLiteral("type"));
    const QJsonArray toolCalls = requiredAction.value(QStringLiteral("submit_tool_outputs"))
                                         .toObject()
                                         .value(QStringLiteral("tool_calls"))
                                         .toArray();
    for (const QJsonValue &value : toolCalls)
        run.d->requiredToolCalls.append(ToolCall::fromJson(value.toObject()));

    const QJsonObject error = json.value(QStringLiteral("last_error")).toObject();
    run.d->errorCode = detail::stringOr(error, QStringLiteral("code"));
    run.d->errorMessage = detail::stringOr(error, QStringLiteral("message"));

    run.d->incompleteDetails = json.value(QStringLiteral("incomplete_details")).toObject();
    run.d->expiresAt = detail::int64Or(json, QStringLiteral("expires_at"));
    run.d->startedAt = detail::int64Or(json, QStringLiteral("started_at"));
    run.d->cancelledAt = detail::int64Or(json, QStringLiteral("cancelled_at"));
    run.d->failedAt = detail::int64Or(json, QStringLiteral("failed_at"));
    run.d->completedAt = detail::int64Or(json, QStringLiteral("completed_at"));
    run.d->model = detail::stringOr(json, QStringLiteral("model"));
    run.d->instructions = detail::stringOr(json, QStringLiteral("instructions"));
    run.d->tools = json.value(QStringLiteral("tools")).toArray();
    run.d->metadata = json.value(QStringLiteral("metadata")).toObject();
    run.d->usage = Usage::fromJson(json.value(QStringLiteral("usage")).toObject());
    run.d->temperature = detail::optionalDouble(json, QStringLiteral("temperature"));
    run.d->topP = detail::optionalDouble(json, QStringLiteral("top_p"));
    run.d->maxPromptTokens = detail::optionalInt(json, QStringLiteral("max_prompt_tokens"));
    run.d->maxCompletionTokens = detail::optionalInt(json, QStringLiteral("max_completion_tokens"));
    run.d->truncationStrategy = json.value(QStringLiteral("truncation_strategy")).toObject();
    const QJsonValue toolChoice = json.value(QStringLiteral("tool_choice"));
    run.d->toolChoice = toolChoice.isNull() ? QJsonValue(QJsonValue::Undefined) : toolChoice;
    run.d->parallelToolCalls = detail::optionalBool(json, QStringLiteral("parallel_tool_calls"));
    const QJsonValue format = json.value(QStringLiteral("response_format"));
    run.d->responseFormat = format.isNull() ? QJsonValue(QJsonValue::Undefined) : format;
    return run;
}

bool Run::operator==(const Run &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->threadId == other.d->threadId
           && d->assistantId == other.d->assistantId && d->status == other.d->status
           && d->requiredActionType == other.d->requiredActionType
           && d->requiredToolCalls == other.d->requiredToolCalls
           && d->errorCode == other.d->errorCode && d->errorMessage == other.d->errorMessage
           && d->incompleteDetails == other.d->incompleteDetails
           && d->expiresAt == other.d->expiresAt && d->startedAt == other.d->startedAt
           && d->cancelledAt == other.d->cancelledAt && d->failedAt == other.d->failedAt
           && d->completedAt == other.d->completedAt && d->model == other.d->model
           && d->instructions == other.d->instructions && d->tools == other.d->tools
           && d->metadata == other.d->metadata && d->usage == other.d->usage
           && d->temperature == other.d->temperature && d->topP == other.d->topP
           && d->maxPromptTokens == other.d->maxPromptTokens
           && d->maxCompletionTokens == other.d->maxCompletionTokens
           && d->truncationStrategy == other.d->truncationStrategy
           && d->toolChoice == other.d->toolChoice
           && d->parallelToolCalls == other.d->parallelToolCalls
           && d->responseFormat == other.d->responseFormat;
}

} // namespace Core
} // namespace QtOpenAi
