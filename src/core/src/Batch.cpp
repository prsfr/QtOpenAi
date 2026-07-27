// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Batch.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

// --- BatchRequestCounts ----------------------------------------------------

QJsonObject BatchRequestCounts::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("total"), total);
    json.insert(QStringLiteral("completed"), completed);
    json.insert(QStringLiteral("failed"), failed);
    return json;
}

BatchRequestCounts BatchRequestCounts::fromJson(const QJsonObject &json)
{
    BatchRequestCounts counts;
    counts.total = json.value(QStringLiteral("total")).toInt();
    counts.completed = json.value(QStringLiteral("completed")).toInt();
    counts.failed = json.value(QStringLiteral("failed")).toInt();
    return counts;
}

// --- BatchError ------------------------------------------------------------

QJsonObject BatchError::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("code"), code);
    detail::insertIfNotEmpty(json, QStringLiteral("message"), message);
    detail::insertIfNotEmpty(json, QStringLiteral("param"), param);
    if (line != 0)
        json.insert(QStringLiteral("line"), line);
    return json;
}

BatchError BatchError::fromJson(const QJsonObject &json)
{
    BatchError error;
    error.code = detail::stringOr(json, QStringLiteral("code"));
    error.message = detail::stringOr(json, QStringLiteral("message"));
    error.param = detail::stringOr(json, QStringLiteral("param"));
    error.line = json.value(QStringLiteral("line")).toInt();
    return error;
}

// --- Batch -----------------------------------------------------------------

class BatchData : public QSharedData
{
public:
    QString id;
    QString object;
    QString endpoint;
    QString inputFileId;
    QString completionWindow;
    BatchStatus status = BatchStatus::Validating;
    QString outputFileId;
    QString errorFileId;
    qint64 createdAt = 0;
    qint64 inProgressAt = 0;
    qint64 expiresAt = 0;
    qint64 finalizingAt = 0;
    qint64 completedAt = 0;
    qint64 failedAt = 0;
    qint64 expiredAt = 0;
    qint64 cancellingAt = 0;
    qint64 cancelledAt = 0;
    BatchRequestCounts requestCounts;
    QList<BatchError> errors;
    QJsonObject metadata;
};

Batch::Batch()
    : d(new BatchData)
{ }

Batch::Batch(const Batch &other) = default;
Batch::Batch(Batch &&other) noexcept = default;
Batch &Batch::operator=(const Batch &other) = default;
Batch &Batch::operator=(Batch &&other) noexcept = default;
Batch::~Batch() = default;

QString Batch::id() const { return d->id; }
void Batch::setId(const QString &id) { d->id = id; }

QString Batch::object() const { return d->object; }
void Batch::setObject(const QString &object) { d->object = object; }

QString Batch::endpoint() const { return d->endpoint; }
void Batch::setEndpoint(const QString &endpoint) { d->endpoint = endpoint; }

QString Batch::inputFileId() const { return d->inputFileId; }
void Batch::setInputFileId(const QString &inputFileId) { d->inputFileId = inputFileId; }

QString Batch::completionWindow() const { return d->completionWindow; }

void Batch::setCompletionWindow(const QString &completionWindow)
{
    d->completionWindow = completionWindow;
}

BatchStatus Batch::status() const { return d->status; }
void Batch::setStatus(BatchStatus status) { d->status = status; }

QString Batch::outputFileId() const { return d->outputFileId; }
void Batch::setOutputFileId(const QString &outputFileId) { d->outputFileId = outputFileId; }

QString Batch::errorFileId() const { return d->errorFileId; }
void Batch::setErrorFileId(const QString &errorFileId) { d->errorFileId = errorFileId; }

qint64 Batch::createdAt() const { return d->createdAt; }
void Batch::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

qint64 Batch::inProgressAt() const { return d->inProgressAt; }
void Batch::setInProgressAt(qint64 inProgressAt) { d->inProgressAt = inProgressAt; }

qint64 Batch::expiresAt() const { return d->expiresAt; }
void Batch::setExpiresAt(qint64 expiresAt) { d->expiresAt = expiresAt; }

qint64 Batch::finalizingAt() const { return d->finalizingAt; }
void Batch::setFinalizingAt(qint64 finalizingAt) { d->finalizingAt = finalizingAt; }

qint64 Batch::completedAt() const { return d->completedAt; }
void Batch::setCompletedAt(qint64 completedAt) { d->completedAt = completedAt; }

qint64 Batch::failedAt() const { return d->failedAt; }
void Batch::setFailedAt(qint64 failedAt) { d->failedAt = failedAt; }

qint64 Batch::expiredAt() const { return d->expiredAt; }
void Batch::setExpiredAt(qint64 expiredAt) { d->expiredAt = expiredAt; }

qint64 Batch::cancellingAt() const { return d->cancellingAt; }
void Batch::setCancellingAt(qint64 cancellingAt) { d->cancellingAt = cancellingAt; }

qint64 Batch::cancelledAt() const { return d->cancelledAt; }
void Batch::setCancelledAt(qint64 cancelledAt) { d->cancelledAt = cancelledAt; }

BatchRequestCounts Batch::requestCounts() const { return d->requestCounts; }

void Batch::setRequestCounts(const BatchRequestCounts &requestCounts)
{
    d->requestCounts = requestCounts;
}

QList<BatchError> Batch::errors() const { return d->errors; }
void Batch::setErrors(const QList<BatchError> &errors) { d->errors = errors; }

QJsonObject Batch::metadata() const { return d->metadata; }
void Batch::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

bool Batch::isTerminal() const { return Core::isTerminal(d->status); }

QJsonObject Batch::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("endpoint"), d->endpoint);
    detail::insertIfNotEmpty(json, QStringLiteral("input_file_id"), d->inputFileId);
    detail::insertIfNotEmpty(json, QStringLiteral("completion_window"), d->completionWindow);
    json.insert(QStringLiteral("status"), batchStatusToString(d->status));
    detail::insertIfNotEmpty(json, QStringLiteral("output_file_id"), d->outputFileId);
    detail::insertIfNotEmpty(json, QStringLiteral("error_file_id"), d->errorFileId);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNonZero(json, QStringLiteral("in_progress_at"), d->inProgressAt);
    detail::insertIfNonZero(json, QStringLiteral("expires_at"), d->expiresAt);
    detail::insertIfNonZero(json, QStringLiteral("finalizing_at"), d->finalizingAt);
    detail::insertIfNonZero(json, QStringLiteral("completed_at"), d->completedAt);
    detail::insertIfNonZero(json, QStringLiteral("failed_at"), d->failedAt);
    detail::insertIfNonZero(json, QStringLiteral("expired_at"), d->expiredAt);
    detail::insertIfNonZero(json, QStringLiteral("cancelling_at"), d->cancellingAt);
    detail::insertIfNonZero(json, QStringLiteral("cancelled_at"), d->cancelledAt);
    json.insert(QStringLiteral("request_counts"), d->requestCounts.toJson());
    if (!d->errors.isEmpty()) {
        QJsonArray data;
        for (const BatchError &error : d->errors)
            data.append(error.toJson());
        json.insert(QStringLiteral("errors"),
                    QJsonObject {{QStringLiteral("object"), QStringLiteral("list")},
                                 {QStringLiteral("data"), data}});
    }
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    return json;
}

Batch Batch::fromJson(const QJsonObject &json)
{
    Batch batch;
    batch.d->id = detail::stringOr(json, QStringLiteral("id"));
    batch.d->object = detail::stringOr(json, QStringLiteral("object"));
    batch.d->endpoint = detail::stringOr(json, QStringLiteral("endpoint"));
    batch.d->inputFileId = detail::stringOr(json, QStringLiteral("input_file_id"));
    batch.d->completionWindow = detail::stringOr(json, QStringLiteral("completion_window"));
    batch.d->status = batchStatusFromString(detail::stringOr(json, QStringLiteral("status")));
    batch.d->outputFileId = detail::stringOr(json, QStringLiteral("output_file_id"));
    batch.d->errorFileId = detail::stringOr(json, QStringLiteral("error_file_id"));
    batch.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    batch.d->inProgressAt = detail::int64Or(json, QStringLiteral("in_progress_at"));
    batch.d->expiresAt = detail::int64Or(json, QStringLiteral("expires_at"));
    batch.d->finalizingAt = detail::int64Or(json, QStringLiteral("finalizing_at"));
    batch.d->completedAt = detail::int64Or(json, QStringLiteral("completed_at"));
    batch.d->failedAt = detail::int64Or(json, QStringLiteral("failed_at"));
    batch.d->expiredAt = detail::int64Or(json, QStringLiteral("expired_at"));
    batch.d->cancellingAt = detail::int64Or(json, QStringLiteral("cancelling_at"));
    batch.d->cancelledAt = detail::int64Or(json, QStringLiteral("cancelled_at"));
    batch.d->requestCounts
            = BatchRequestCounts::fromJson(json.value(QStringLiteral("request_counts")).toObject());
    // `errors` is itself a list object, so the entries sit one level down.
    const QJsonArray errors = json.value(QStringLiteral("errors"))
                                      .toObject()
                                      .value(QStringLiteral("data"))
                                      .toArray();
    for (const QJsonValue &value : errors)
        batch.d->errors.append(BatchError::fromJson(value.toObject()));
    batch.d->metadata = json.value(QStringLiteral("metadata")).toObject();
    return batch;
}

bool Batch::operator==(const Batch &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->endpoint == other.d->endpoint
           && d->inputFileId == other.d->inputFileId
           && d->completionWindow == other.d->completionWindow && d->status == other.d->status
           && d->outputFileId == other.d->outputFileId && d->errorFileId == other.d->errorFileId
           && d->createdAt == other.d->createdAt && d->inProgressAt == other.d->inProgressAt
           && d->expiresAt == other.d->expiresAt && d->finalizingAt == other.d->finalizingAt
           && d->completedAt == other.d->completedAt && d->failedAt == other.d->failedAt
           && d->expiredAt == other.d->expiredAt && d->cancellingAt == other.d->cancellingAt
           && d->cancelledAt == other.d->cancelledAt && d->requestCounts == other.d->requestCounts
           && d->errors == other.d->errors && d->metadata == other.d->metadata;
}

} // namespace Core
} // namespace QtOpenAi
