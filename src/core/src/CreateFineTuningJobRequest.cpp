// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/CreateFineTuningJobRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class CreateFineTuningJobRequestData : public QSharedData
{
public:
    QString model;
    QString trainingFile;
    QString validationFile;
    QString suffix;
    int seed = 0;
    QString methodType;
    FineTuningHyperparameters hyperparameters;
    QJsonObject metadata;
};

CreateFineTuningJobRequest::CreateFineTuningJobRequest()
    : d(new CreateFineTuningJobRequestData)
{ }

CreateFineTuningJobRequest::CreateFineTuningJobRequest(QString model, QString trainingFile)
    : d(new CreateFineTuningJobRequestData)
{
    d->model = std::move(model);
    d->trainingFile = std::move(trainingFile);
}

CreateFineTuningJobRequest::CreateFineTuningJobRequest(const CreateFineTuningJobRequest &other)
        = default;
CreateFineTuningJobRequest::CreateFineTuningJobRequest(CreateFineTuningJobRequest &&other) noexcept
        = default;
CreateFineTuningJobRequest &
CreateFineTuningJobRequest::operator=(const CreateFineTuningJobRequest &other)
        = default;
CreateFineTuningJobRequest &
CreateFineTuningJobRequest::operator=(CreateFineTuningJobRequest &&other) noexcept
        = default;
CreateFineTuningJobRequest::~CreateFineTuningJobRequest() = default;

QString CreateFineTuningJobRequest::model() const { return d->model; }
void CreateFineTuningJobRequest::setModel(const QString &model) { d->model = model; }

QString CreateFineTuningJobRequest::trainingFile() const { return d->trainingFile; }
void CreateFineTuningJobRequest::setTrainingFile(const QString &trainingFile)
{
    d->trainingFile = trainingFile;
}

QString CreateFineTuningJobRequest::validationFile() const { return d->validationFile; }
void CreateFineTuningJobRequest::setValidationFile(const QString &validationFile)
{
    d->validationFile = validationFile;
}

QString CreateFineTuningJobRequest::suffix() const { return d->suffix; }
void CreateFineTuningJobRequest::setSuffix(const QString &suffix) { d->suffix = suffix; }

int CreateFineTuningJobRequest::seed() const { return d->seed; }
void CreateFineTuningJobRequest::setSeed(int seed) { d->seed = seed; }

QString CreateFineTuningJobRequest::methodType() const { return d->methodType; }
void CreateFineTuningJobRequest::setMethodType(const QString &methodType)
{
    d->methodType = methodType;
}

FineTuningHyperparameters CreateFineTuningJobRequest::hyperparameters() const
{
    return d->hyperparameters;
}

void CreateFineTuningJobRequest::setHyperparameters(
        const FineTuningHyperparameters &hyperparameters)
{
    d->hyperparameters = hyperparameters;
}

QJsonObject CreateFineTuningJobRequest::metadata() const { return d->metadata; }
void CreateFineTuningJobRequest::setMetadata(const QJsonObject &metadata)
{
    d->metadata = metadata;
}

QJsonObject CreateFineTuningJobRequest::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("model"), d->model);
    detail::insertIfNotEmpty(json, QStringLiteral("training_file"), d->trainingFile);
    detail::insertIfNotEmpty(json, QStringLiteral("validation_file"), d->validationFile);
    detail::insertIfNotEmpty(json, QStringLiteral("suffix"), d->suffix);
    if (d->seed != 0)
        json.insert(QStringLiteral("seed"), d->seed);
    if (!d->methodType.isEmpty())
        json.insert(QStringLiteral("method"), d->hyperparameters.toMethodJson(d->methodType));
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    return json;
}

bool CreateFineTuningJobRequest::operator==(const CreateFineTuningJobRequest &other) const
{
    return d->model == other.d->model && d->trainingFile == other.d->trainingFile
           && d->validationFile == other.d->validationFile && d->suffix == other.d->suffix
           && d->seed == other.d->seed && d->methodType == other.d->methodType
           && d->hyperparameters == other.d->hyperparameters && d->metadata == other.d->metadata;
}

} // namespace Core
} // namespace QtOpenAi
