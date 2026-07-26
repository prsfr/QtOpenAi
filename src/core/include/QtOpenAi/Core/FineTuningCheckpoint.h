// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

// The training/validation metrics captured at a checkpoint. A plain value
// aggregate like BatchRequestCounts: a fixed set of scalars with no growth path.
struct QTOPENAI_CORE_EXPORT FineTuningCheckpointMetrics
{
    double step = 0.0;
    double trainLoss = 0.0;
    double trainMeanTokenAccuracy = 0.0;
    double validLoss = 0.0;
    double validMeanTokenAccuracy = 0.0;
    double fullValidLoss = 0.0;
    double fullValidMeanTokenAccuracy = 0.0;

    QJsonObject toJson() const;
    static FineTuningCheckpointMetrics fromJson(const QJsonObject &json);

    bool operator==(const FineTuningCheckpointMetrics &other) const
    {
        return qFuzzyCompare(step, other.step) && qFuzzyCompare(trainLoss, other.trainLoss)
               && qFuzzyCompare(trainMeanTokenAccuracy, other.trainMeanTokenAccuracy)
               && qFuzzyCompare(validLoss, other.validLoss)
               && qFuzzyCompare(validMeanTokenAccuracy, other.validMeanTokenAccuracy)
               && qFuzzyCompare(fullValidLoss, other.fullValidLoss)
               && qFuzzyCompare(fullValidMeanTokenAccuracy, other.fullValidMeanTokenAccuracy);
    }
    bool operator!=(const FineTuningCheckpointMetrics &other) const { return !(*this == other); }
};

class FineTuningJobCheckpointData;

// A snapshot of a fine-tuning job taken mid-training
// (GET /fine_tuning/jobs/{id}/checkpoints). Each checkpoint names a usable model
// so an earlier, better-performing epoch can be picked over the final one.
class QTOPENAI_CORE_EXPORT FineTuningJobCheckpoint
{
public:
    FineTuningJobCheckpoint();
    FineTuningJobCheckpoint(const FineTuningJobCheckpoint &other);
    FineTuningJobCheckpoint(FineTuningJobCheckpoint &&other) noexcept;
    FineTuningJobCheckpoint &operator=(const FineTuningJobCheckpoint &other);
    FineTuningJobCheckpoint &operator=(FineTuningJobCheckpoint &&other) noexcept;
    ~FineTuningJobCheckpoint();

    void swap(FineTuningJobCheckpoint &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "fine_tuning.job.checkpoint".
    QString object() const;
    void setObject(const QString &object);

    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    // The model name this checkpoint can be used under.
    QString fineTunedModelCheckpoint() const;
    void setFineTunedModelCheckpoint(const QString &fineTunedModelCheckpoint);

    QString fineTuningJobId() const;
    void setFineTuningJobId(const QString &fineTuningJobId);

    // Training step the checkpoint was taken at.
    int stepNumber() const;
    void setStepNumber(int stepNumber);

    FineTuningCheckpointMetrics metrics() const;
    void setMetrics(const FineTuningCheckpointMetrics &metrics);

    QJsonObject toJson() const;
    static FineTuningJobCheckpoint fromJson(const QJsonObject &json);

    bool operator==(const FineTuningJobCheckpoint &other) const;
    bool operator!=(const FineTuningJobCheckpoint &other) const { return !(*this == other); }

private:
    QSharedDataPointer<FineTuningJobCheckpointData> d;
};

// A `list` of checkpoints (GET /fine_tuning/jobs/{id}/checkpoints).
using FineTuningJobCheckpointList = ListPage<FineTuningJobCheckpoint>;

class FineTuningCheckpointPermissionData;

// A grant letting one project use a fine-tuned checkpoint
// (/fine_tuning/checkpoints/{checkpoint}/permissions). The deletion
// acknowledgement shares this shape, with object "checkpoint.permission.deleted".
class QTOPENAI_CORE_EXPORT FineTuningCheckpointPermission
{
public:
    FineTuningCheckpointPermission();
    FineTuningCheckpointPermission(const FineTuningCheckpointPermission &other);
    FineTuningCheckpointPermission(FineTuningCheckpointPermission &&other) noexcept;
    FineTuningCheckpointPermission &operator=(const FineTuningCheckpointPermission &other);
    FineTuningCheckpointPermission &operator=(FineTuningCheckpointPermission &&other) noexcept;
    ~FineTuningCheckpointPermission();

    void swap(FineTuningCheckpointPermission &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "checkpoint.permission".
    QString object() const;
    void setObject(const QString &object);

    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    QString projectId() const;
    void setProjectId(const QString &projectId);

    QJsonObject toJson() const;
    static FineTuningCheckpointPermission fromJson(const QJsonObject &json);

    bool operator==(const FineTuningCheckpointPermission &other) const;
    bool operator!=(const FineTuningCheckpointPermission &other) const { return !(*this == other); }

private:
    QSharedDataPointer<FineTuningCheckpointPermissionData> d;
};

// A `list` of checkpoint permissions.
using FineTuningCheckpointPermissionList = ListPage<FineTuningCheckpointPermission>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::FineTuningJobCheckpoint)
Q_DECLARE_SHARED(QtOpenAi::Core::FineTuningCheckpointPermission)
Q_DECLARE_METATYPE(QtOpenAi::Core::FineTuningJobCheckpoint)
Q_DECLARE_METATYPE(QtOpenAi::Core::FineTuningJobCheckpointList)
Q_DECLARE_METATYPE(QtOpenAi::Core::FineTuningCheckpointPermission)
Q_DECLARE_METATYPE(QtOpenAi::Core::FineTuningCheckpointPermissionList)
