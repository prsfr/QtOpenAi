// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/FineTuningCheckpoint.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

// --- FineTuningCheckpointMetrics -------------------------------------------

QJsonObject FineTuningCheckpointMetrics::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("step"), step);
    json.insert(QStringLiteral("train_loss"), trainLoss);
    json.insert(QStringLiteral("train_mean_token_accuracy"), trainMeanTokenAccuracy);
    json.insert(QStringLiteral("valid_loss"), validLoss);
    json.insert(QStringLiteral("valid_mean_token_accuracy"), validMeanTokenAccuracy);
    json.insert(QStringLiteral("full_valid_loss"), fullValidLoss);
    json.insert(QStringLiteral("full_valid_mean_token_accuracy"), fullValidMeanTokenAccuracy);
    return json;
}

FineTuningCheckpointMetrics FineTuningCheckpointMetrics::fromJson(const QJsonObject &json)
{
    FineTuningCheckpointMetrics metrics;
    metrics.step = json.value(QStringLiteral("step")).toDouble();
    metrics.trainLoss = json.value(QStringLiteral("train_loss")).toDouble();
    metrics.trainMeanTokenAccuracy
            = json.value(QStringLiteral("train_mean_token_accuracy")).toDouble();
    metrics.validLoss = json.value(QStringLiteral("valid_loss")).toDouble();
    metrics.validMeanTokenAccuracy
            = json.value(QStringLiteral("valid_mean_token_accuracy")).toDouble();
    metrics.fullValidLoss = json.value(QStringLiteral("full_valid_loss")).toDouble();
    metrics.fullValidMeanTokenAccuracy
            = json.value(QStringLiteral("full_valid_mean_token_accuracy")).toDouble();
    return metrics;
}

// --- FineTuningJobCheckpoint -----------------------------------------------

class FineTuningJobCheckpointData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    QString fineTunedModelCheckpoint;
    QString fineTuningJobId;
    int stepNumber = 0;
    FineTuningCheckpointMetrics metrics;
};

FineTuningJobCheckpoint::FineTuningJobCheckpoint()
    : d(new FineTuningJobCheckpointData)
{ }

FineTuningJobCheckpoint::FineTuningJobCheckpoint(const FineTuningJobCheckpoint &other) = default;
FineTuningJobCheckpoint::FineTuningJobCheckpoint(FineTuningJobCheckpoint &&other) noexcept
        = default;
FineTuningJobCheckpoint &FineTuningJobCheckpoint::operator=(const FineTuningJobCheckpoint &other)
        = default;
FineTuningJobCheckpoint &
FineTuningJobCheckpoint::operator=(FineTuningJobCheckpoint &&other) noexcept
        = default;
FineTuningJobCheckpoint::~FineTuningJobCheckpoint() = default;

QString FineTuningJobCheckpoint::id() const { return d->id; }
void FineTuningJobCheckpoint::setId(const QString &id) { d->id = id; }

QString FineTuningJobCheckpoint::object() const { return d->object; }
void FineTuningJobCheckpoint::setObject(const QString &object) { d->object = object; }

qint64 FineTuningJobCheckpoint::createdAt() const { return d->createdAt; }
void FineTuningJobCheckpoint::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString FineTuningJobCheckpoint::fineTunedModelCheckpoint() const
{
    return d->fineTunedModelCheckpoint;
}

void FineTuningJobCheckpoint::setFineTunedModelCheckpoint(const QString &fineTunedModelCheckpoint)
{
    d->fineTunedModelCheckpoint = fineTunedModelCheckpoint;
}

QString FineTuningJobCheckpoint::fineTuningJobId() const { return d->fineTuningJobId; }
void FineTuningJobCheckpoint::setFineTuningJobId(const QString &fineTuningJobId)
{
    d->fineTuningJobId = fineTuningJobId;
}

int FineTuningJobCheckpoint::stepNumber() const { return d->stepNumber; }
void FineTuningJobCheckpoint::setStepNumber(int stepNumber) { d->stepNumber = stepNumber; }

FineTuningCheckpointMetrics FineTuningJobCheckpoint::metrics() const { return d->metrics; }
void FineTuningJobCheckpoint::setMetrics(const FineTuningCheckpointMetrics &metrics)
{
    d->metrics = metrics;
}

QJsonObject FineTuningJobCheckpoint::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("fine_tuned_model_checkpoint"),
                             d->fineTunedModelCheckpoint);
    detail::insertIfNotEmpty(json, QStringLiteral("fine_tuning_job_id"), d->fineTuningJobId);
    json.insert(QStringLiteral("step_number"), d->stepNumber);
    json.insert(QStringLiteral("metrics"), d->metrics.toJson());
    return json;
}

FineTuningJobCheckpoint FineTuningJobCheckpoint::fromJson(const QJsonObject &json)
{
    FineTuningJobCheckpoint checkpoint;
    checkpoint.d->id = detail::stringOr(json, QStringLiteral("id"));
    checkpoint.d->object = detail::stringOr(json, QStringLiteral("object"));
    checkpoint.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    checkpoint.d->fineTunedModelCheckpoint
            = detail::stringOr(json, QStringLiteral("fine_tuned_model_checkpoint"));
    checkpoint.d->fineTuningJobId = detail::stringOr(json, QStringLiteral("fine_tuning_job_id"));
    checkpoint.d->stepNumber = json.value(QStringLiteral("step_number")).toInt();
    checkpoint.d->metrics = FineTuningCheckpointMetrics::fromJson(
            json.value(QStringLiteral("metrics")).toObject());
    return checkpoint;
}

bool FineTuningJobCheckpoint::operator==(const FineTuningJobCheckpoint &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt
           && d->fineTunedModelCheckpoint == other.d->fineTunedModelCheckpoint
           && d->fineTuningJobId == other.d->fineTuningJobId && d->stepNumber == other.d->stepNumber
           && d->metrics == other.d->metrics;
}

// --- FineTuningCheckpointPermission ----------------------------------------

class FineTuningCheckpointPermissionData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    QString projectId;
};

FineTuningCheckpointPermission::FineTuningCheckpointPermission()
    : d(new FineTuningCheckpointPermissionData)
{ }

FineTuningCheckpointPermission::FineTuningCheckpointPermission(
        const FineTuningCheckpointPermission &other)
        = default;
FineTuningCheckpointPermission::FineTuningCheckpointPermission(
        FineTuningCheckpointPermission &&other) noexcept
        = default;
FineTuningCheckpointPermission &
FineTuningCheckpointPermission::operator=(const FineTuningCheckpointPermission &other)
        = default;
FineTuningCheckpointPermission &
FineTuningCheckpointPermission::operator=(FineTuningCheckpointPermission &&other) noexcept
        = default;
FineTuningCheckpointPermission::~FineTuningCheckpointPermission() = default;

QString FineTuningCheckpointPermission::id() const { return d->id; }
void FineTuningCheckpointPermission::setId(const QString &id) { d->id = id; }

QString FineTuningCheckpointPermission::object() const { return d->object; }
void FineTuningCheckpointPermission::setObject(const QString &object) { d->object = object; }

qint64 FineTuningCheckpointPermission::createdAt() const { return d->createdAt; }
void FineTuningCheckpointPermission::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString FineTuningCheckpointPermission::projectId() const { return d->projectId; }
void FineTuningCheckpointPermission::setProjectId(const QString &projectId)
{
    d->projectId = projectId;
}

QJsonObject FineTuningCheckpointPermission::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("project_id"), d->projectId);
    return json;
}

FineTuningCheckpointPermission FineTuningCheckpointPermission::fromJson(const QJsonObject &json)
{
    FineTuningCheckpointPermission permission;
    permission.d->id = detail::stringOr(json, QStringLiteral("id"));
    permission.d->object = detail::stringOr(json, QStringLiteral("object"));
    permission.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    permission.d->projectId = detail::stringOr(json, QStringLiteral("project_id"));
    return permission;
}

bool FineTuningCheckpointPermission::operator==(const FineTuningCheckpointPermission &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->projectId == other.d->projectId;
}

} // namespace Core
} // namespace QtOpenAi
