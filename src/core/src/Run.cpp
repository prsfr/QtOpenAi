// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Run.h"

#include "JsonHelpers_p.h"
#include "RunFields_p.h"

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

// The thirteen fields a run shares with the body that creates one come from the
// private base; only what the server assigns lives here. See RunFields_p.h.
class RunData : public detail::RunFieldsData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    QString threadId;
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
    Usage usage;
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
    detail::insertRunFields(json, *d);
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
    json.insert(QStringLiteral("usage"), d->usage.toJson());
    return json;
}

Run Run::fromJson(const QJsonObject &json)
{
    Run run;
    run.d->id = detail::stringOr(json, QStringLiteral("id"));
    run.d->object = detail::stringOr(json, QStringLiteral("object"));
    run.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    run.d->threadId = detail::stringOr(json, QStringLiteral("thread_id"));
    detail::readRunFields(*run.d, json);
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
    run.d->usage = Usage::fromJson(json.value(QStringLiteral("usage")).toObject());
    return run;
}

bool Run::operator==(const Run &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->threadId == other.d->threadId
           && d->status == other.d->status && d->requiredActionType == other.d->requiredActionType
           && d->requiredToolCalls == other.d->requiredToolCalls
           && d->errorCode == other.d->errorCode && d->errorMessage == other.d->errorMessage
           && d->incompleteDetails == other.d->incompleteDetails
           && d->expiresAt == other.d->expiresAt && d->startedAt == other.d->startedAt
           && d->cancelledAt == other.d->cancelledAt && d->failedAt == other.d->failedAt
           && d->completedAt == other.d->completedAt && d->usage == other.d->usage
           && detail::runFieldsEqual(*d, *other.d);
}

} // namespace Core
} // namespace QtOpenAi
