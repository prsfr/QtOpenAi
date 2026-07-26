// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/FineTuningJob.h>
#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class CreateFineTuningJobRequestData;

// The body of a POST /fine_tuning/jobs request.
//
// Only the base model and the training file are required; every optional field
// stays out of the body so the service applies its own default, which for the
// hyperparameters means choosing them automatically.
class QTOPENAI_CORE_EXPORT CreateFineTuningJobRequest
{
public:
    CreateFineTuningJobRequest();
    CreateFineTuningJobRequest(QString model, QString trainingFile);
    CreateFineTuningJobRequest(const CreateFineTuningJobRequest &other);
    CreateFineTuningJobRequest(CreateFineTuningJobRequest &&other) noexcept;
    CreateFineTuningJobRequest &operator=(const CreateFineTuningJobRequest &other);
    CreateFineTuningJobRequest &operator=(CreateFineTuningJobRequest &&other) noexcept;
    ~CreateFineTuningJobRequest();

    void swap(CreateFineTuningJobRequest &other) noexcept { d.swap(other.d); }

    QString model() const;
    void setModel(const QString &model);

    // Files API id of the training data (JSONL, purpose "fine-tune").
    QString trainingFile() const;
    void setTrainingFile(const QString &trainingFile);

    QString validationFile() const;
    void setValidationFile(const QString &validationFile);

    // Appended to the generated model's name to make runs recognisable.
    QString suffix() const;
    void setSuffix(const QString &suffix);

    // Seed for reproducible runs; omitted while 0.
    int seed() const;
    void setSeed(int seed);

    // Training method, e.g. "supervised" or "dpo". The hyperparameters are sent
    // nested under it, so setting them without a method type has no effect.
    QString methodType() const;
    void setMethodType(const QString &methodType);

    FineTuningHyperparameters hyperparameters() const;
    void setHyperparameters(const FineTuningHyperparameters &hyperparameters);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    QJsonObject toJson() const;

    bool operator==(const CreateFineTuningJobRequest &other) const;
    bool operator!=(const CreateFineTuningJobRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CreateFineTuningJobRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::CreateFineTuningJobRequest)
