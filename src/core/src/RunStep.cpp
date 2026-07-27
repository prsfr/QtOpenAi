// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/RunStep.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class RunStepData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    QString assistantId;
    QString threadId;
    QString runId;
    QString type;
    RunStepStatus status = RunStepStatus::InProgress;
    QJsonObject stepDetails;
    QString errorCode;
    QString errorMessage;
    qint64 expiredAt = 0;
    qint64 cancelledAt = 0;
    qint64 failedAt = 0;
    qint64 completedAt = 0;
    Usage usage;
    QJsonObject metadata;
};

RunStep::RunStep()
    : d(new RunStepData)
{ }

RunStep::RunStep(const RunStep &other) = default;
RunStep::RunStep(RunStep &&other) noexcept = default;
RunStep &RunStep::operator=(const RunStep &other) = default;
RunStep &RunStep::operator=(RunStep &&other) noexcept = default;
RunStep::~RunStep() = default;

QString RunStep::id() const { return d->id; }
void RunStep::setId(const QString &id) { d->id = id; }

QString RunStep::object() const { return d->object; }
void RunStep::setObject(const QString &object) { d->object = object; }

qint64 RunStep::createdAt() const { return d->createdAt; }
void RunStep::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString RunStep::assistantId() const { return d->assistantId; }
void RunStep::setAssistantId(const QString &assistantId) { d->assistantId = assistantId; }

QString RunStep::threadId() const { return d->threadId; }
void RunStep::setThreadId(const QString &threadId) { d->threadId = threadId; }

QString RunStep::runId() const { return d->runId; }
void RunStep::setRunId(const QString &runId) { d->runId = runId; }

QString RunStep::type() const { return d->type; }
void RunStep::setType(const QString &type) { d->type = type; }

RunStepStatus RunStep::status() const { return d->status; }
void RunStep::setStatus(RunStepStatus status) { d->status = status; }

QJsonObject RunStep::stepDetails() const { return d->stepDetails; }
void RunStep::setStepDetails(const QJsonObject &stepDetails) { d->stepDetails = stepDetails; }

QString RunStep::errorCode() const { return d->errorCode; }
void RunStep::setErrorCode(const QString &errorCode) { d->errorCode = errorCode; }

QString RunStep::errorMessage() const { return d->errorMessage; }
void RunStep::setErrorMessage(const QString &errorMessage) { d->errorMessage = errorMessage; }

qint64 RunStep::expiredAt() const { return d->expiredAt; }
void RunStep::setExpiredAt(qint64 expiredAt) { d->expiredAt = expiredAt; }

qint64 RunStep::cancelledAt() const { return d->cancelledAt; }
void RunStep::setCancelledAt(qint64 cancelledAt) { d->cancelledAt = cancelledAt; }

qint64 RunStep::failedAt() const { return d->failedAt; }
void RunStep::setFailedAt(qint64 failedAt) { d->failedAt = failedAt; }

qint64 RunStep::completedAt() const { return d->completedAt; }
void RunStep::setCompletedAt(qint64 completedAt) { d->completedAt = completedAt; }

Usage RunStep::usage() const { return d->usage; }
void RunStep::setUsage(const Usage &usage) { d->usage = usage; }

QJsonObject RunStep::metadata() const { return d->metadata; }
void RunStep::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

bool RunStep::isTerminal() const
{
    switch (d->status) {
    case RunStepStatus::Cancelled:
    case RunStepStatus::Failed:
    case RunStepStatus::Completed:
    case RunStepStatus::Expired:
        return true;
    case RunStepStatus::InProgress:
        return false;
    }
    return false;
}

QJsonObject RunStep::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("assistant_id"), d->assistantId);
    detail::insertIfNotEmpty(json, QStringLiteral("thread_id"), d->threadId);
    detail::insertIfNotEmpty(json, QStringLiteral("run_id"), d->runId);
    detail::insertIfNotEmpty(json, QStringLiteral("type"), d->type);
    json.insert(QStringLiteral("status"), runStepStatusToString(d->status));
    if (!d->stepDetails.isEmpty())
        json.insert(QStringLiteral("step_details"), d->stepDetails);
    if (!d->errorCode.isEmpty() || !d->errorMessage.isEmpty()) {
        QJsonObject error;
        detail::insertIfNotEmpty(error, QStringLiteral("code"), d->errorCode);
        detail::insertIfNotEmpty(error, QStringLiteral("message"), d->errorMessage);
        json.insert(QStringLiteral("last_error"), error);
    }
    detail::insertIfNonZero(json, QStringLiteral("expired_at"), d->expiredAt);
    detail::insertIfNonZero(json, QStringLiteral("cancelled_at"), d->cancelledAt);
    detail::insertIfNonZero(json, QStringLiteral("failed_at"), d->failedAt);
    detail::insertIfNonZero(json, QStringLiteral("completed_at"), d->completedAt);
    json.insert(QStringLiteral("usage"), d->usage.toJson());
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    return json;
}

RunStep RunStep::fromJson(const QJsonObject &json)
{
    RunStep step;
    step.d->id = detail::stringOr(json, QStringLiteral("id"));
    step.d->object = detail::stringOr(json, QStringLiteral("object"));
    step.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    step.d->assistantId = detail::stringOr(json, QStringLiteral("assistant_id"));
    step.d->threadId = detail::stringOr(json, QStringLiteral("thread_id"));
    step.d->runId = detail::stringOr(json, QStringLiteral("run_id"));
    step.d->type = detail::stringOr(json, QStringLiteral("type"));
    step.d->status = runStepStatusFromString(detail::stringOr(json, QStringLiteral("status")));
    step.d->stepDetails = json.value(QStringLiteral("step_details")).toObject();

    const QJsonObject error = json.value(QStringLiteral("last_error")).toObject();
    step.d->errorCode = detail::stringOr(error, QStringLiteral("code"));
    step.d->errorMessage = detail::stringOr(error, QStringLiteral("message"));

    step.d->expiredAt = detail::int64Or(json, QStringLiteral("expired_at"));
    step.d->cancelledAt = detail::int64Or(json, QStringLiteral("cancelled_at"));
    step.d->failedAt = detail::int64Or(json, QStringLiteral("failed_at"));
    step.d->completedAt = detail::int64Or(json, QStringLiteral("completed_at"));
    step.d->usage = Usage::fromJson(json.value(QStringLiteral("usage")).toObject());
    step.d->metadata = json.value(QStringLiteral("metadata")).toObject();
    return step;
}

bool RunStep::operator==(const RunStep &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->assistantId == other.d->assistantId
           && d->threadId == other.d->threadId && d->runId == other.d->runId
           && d->type == other.d->type && d->status == other.d->status
           && d->stepDetails == other.d->stepDetails && d->errorCode == other.d->errorCode
           && d->errorMessage == other.d->errorMessage && d->expiredAt == other.d->expiredAt
           && d->cancelledAt == other.d->cancelledAt && d->failedAt == other.d->failedAt
           && d->completedAt == other.d->completedAt && d->usage == other.d->usage
           && d->metadata == other.d->metadata;
}

} // namespace Core
} // namespace QtOpenAi
