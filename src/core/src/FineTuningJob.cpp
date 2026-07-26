// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/FineTuningJob.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

namespace {

// Read a hyperparameter the service may have chosen itself: anything that is not
// a number (in practice the string "auto") means "unset".
std::optional<int> autoOrInt(const QJsonObject &json, const QString &key)
{
    const QJsonValue value = json.value(key);
    return value.isDouble() ? std::optional<int>(value.toInt()) : std::nullopt;
}

std::optional<double> autoOrDouble(const QJsonObject &json, const QString &key)
{
    const QJsonValue value = json.value(key);
    return value.isDouble() ? std::optional<double>(value.toDouble()) : std::nullopt;
}

} // namespace

// --- FineTuningHyperparameters ---------------------------------------------

QJsonObject FineTuningHyperparameters::toJson() const
{
    QJsonObject json;
    if (nEpochs)
        json.insert(QStringLiteral("n_epochs"), *nEpochs);
    if (batchSize)
        json.insert(QStringLiteral("batch_size"), *batchSize);
    if (learningRateMultiplier)
        json.insert(QStringLiteral("learning_rate_multiplier"), *learningRateMultiplier);
    return json;
}

FineTuningHyperparameters FineTuningHyperparameters::fromJson(const QJsonObject &json)
{
    FineTuningHyperparameters hyperparameters;
    hyperparameters.nEpochs = autoOrInt(json, QStringLiteral("n_epochs"));
    hyperparameters.batchSize = autoOrInt(json, QStringLiteral("batch_size"));
    hyperparameters.learningRateMultiplier
            = autoOrDouble(json, QStringLiteral("learning_rate_multiplier"));
    return hyperparameters;
}

QJsonObject FineTuningHyperparameters::toMethodJson(const QString &methodType) const
{
    const QJsonObject hyperparameters = toJson();
    QJsonObject method {{QStringLiteral("type"), methodType}};
    if (!hyperparameters.isEmpty())
        method.insert(methodType,
                      QJsonObject {{QStringLiteral("hyperparameters"), hyperparameters}});
    return method;
}

FineTuningHyperparameters FineTuningHyperparameters::fromMethodJson(const QJsonObject &method)
{
    const QString type = detail::stringOr(method, QStringLiteral("type"));
    return fromJson(
            method.value(type).toObject().value(QStringLiteral("hyperparameters")).toObject());
}

// --- FineTuningJob ---------------------------------------------------------

class FineTuningJobData : public QSharedData
{
public:
    QString id;
    QString object;
    QString model;
    qint64 createdAt = 0;
    qint64 finishedAt = 0;
    QString fineTunedModel;
    QString organizationId;
    QString trainingFile;
    QString validationFile;
    QStringList resultFiles;
    FineTuningJobStatus status = FineTuningJobStatus::Queued;
    qint64 trainedTokens = 0;
    int seed = 0;
    QString methodType;
    FineTuningHyperparameters hyperparameters;
    QString errorCode;
    QString errorMessage;
    QString errorParam;
    QJsonObject metadata;
};

FineTuningJob::FineTuningJob()
    : d(new FineTuningJobData)
{ }

FineTuningJob::FineTuningJob(const FineTuningJob &other) = default;
FineTuningJob::FineTuningJob(FineTuningJob &&other) noexcept = default;
FineTuningJob &FineTuningJob::operator=(const FineTuningJob &other) = default;
FineTuningJob &FineTuningJob::operator=(FineTuningJob &&other) noexcept = default;
FineTuningJob::~FineTuningJob() = default;

QString FineTuningJob::id() const { return d->id; }
void FineTuningJob::setId(const QString &id) { d->id = id; }

QString FineTuningJob::object() const { return d->object; }
void FineTuningJob::setObject(const QString &object) { d->object = object; }

QString FineTuningJob::model() const { return d->model; }
void FineTuningJob::setModel(const QString &model) { d->model = model; }

qint64 FineTuningJob::createdAt() const { return d->createdAt; }
void FineTuningJob::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

qint64 FineTuningJob::finishedAt() const { return d->finishedAt; }
void FineTuningJob::setFinishedAt(qint64 finishedAt) { d->finishedAt = finishedAt; }

QString FineTuningJob::fineTunedModel() const { return d->fineTunedModel; }
void FineTuningJob::setFineTunedModel(const QString &fineTunedModel)
{
    d->fineTunedModel = fineTunedModel;
}

QString FineTuningJob::organizationId() const { return d->organizationId; }
void FineTuningJob::setOrganizationId(const QString &organizationId)
{
    d->organizationId = organizationId;
}

QString FineTuningJob::trainingFile() const { return d->trainingFile; }
void FineTuningJob::setTrainingFile(const QString &trainingFile) { d->trainingFile = trainingFile; }

QString FineTuningJob::validationFile() const { return d->validationFile; }
void FineTuningJob::setValidationFile(const QString &validationFile)
{
    d->validationFile = validationFile;
}

QStringList FineTuningJob::resultFiles() const { return d->resultFiles; }
void FineTuningJob::setResultFiles(const QStringList &resultFiles) { d->resultFiles = resultFiles; }

FineTuningJobStatus FineTuningJob::status() const { return d->status; }
void FineTuningJob::setStatus(FineTuningJobStatus status) { d->status = status; }

qint64 FineTuningJob::trainedTokens() const { return d->trainedTokens; }
void FineTuningJob::setTrainedTokens(qint64 trainedTokens) { d->trainedTokens = trainedTokens; }

int FineTuningJob::seed() const { return d->seed; }
void FineTuningJob::setSeed(int seed) { d->seed = seed; }

QString FineTuningJob::methodType() const { return d->methodType; }
void FineTuningJob::setMethodType(const QString &methodType) { d->methodType = methodType; }

FineTuningHyperparameters FineTuningJob::hyperparameters() const { return d->hyperparameters; }
void FineTuningJob::setHyperparameters(const FineTuningHyperparameters &hyperparameters)
{
    d->hyperparameters = hyperparameters;
}

QString FineTuningJob::errorCode() const { return d->errorCode; }
void FineTuningJob::setErrorCode(const QString &errorCode) { d->errorCode = errorCode; }

QString FineTuningJob::errorMessage() const { return d->errorMessage; }
void FineTuningJob::setErrorMessage(const QString &errorMessage) { d->errorMessage = errorMessage; }

QString FineTuningJob::errorParam() const { return d->errorParam; }
void FineTuningJob::setErrorParam(const QString &errorParam) { d->errorParam = errorParam; }

QJsonObject FineTuningJob::metadata() const { return d->metadata; }
void FineTuningJob::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

bool FineTuningJob::isTerminal() const
{
    switch (d->status) {
    case FineTuningJobStatus::Succeeded:
    case FineTuningJobStatus::Failed:
    case FineTuningJobStatus::Cancelled:
        return true;
    case FineTuningJobStatus::ValidatingFiles:
    case FineTuningJobStatus::Queued:
    case FineTuningJobStatus::Running:
    case FineTuningJobStatus::Paused:
        return false;
    }
    return false;
}

QJsonObject FineTuningJob::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("model"), d->model);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNonZero(json, QStringLiteral("finished_at"), d->finishedAt);
    detail::insertIfNotEmpty(json, QStringLiteral("fine_tuned_model"), d->fineTunedModel);
    detail::insertIfNotEmpty(json, QStringLiteral("organization_id"), d->organizationId);
    detail::insertIfNotEmpty(json, QStringLiteral("training_file"), d->trainingFile);
    detail::insertIfNotEmpty(json, QStringLiteral("validation_file"), d->validationFile);
    if (!d->resultFiles.isEmpty())
        json.insert(QStringLiteral("result_files"), QJsonArray::fromStringList(d->resultFiles));
    json.insert(QStringLiteral("status"), fineTuningJobStatusToString(d->status));
    detail::insertIfNonZero(json, QStringLiteral("trained_tokens"), d->trainedTokens);
    if (d->seed != 0)
        json.insert(QStringLiteral("seed"), d->seed);
    if (!d->methodType.isEmpty())
        json.insert(QStringLiteral("method"), d->hyperparameters.toMethodJson(d->methodType));
    if (!d->errorCode.isEmpty() || !d->errorMessage.isEmpty() || !d->errorParam.isEmpty()) {
        QJsonObject error;
        detail::insertIfNotEmpty(error, QStringLiteral("code"), d->errorCode);
        detail::insertIfNotEmpty(error, QStringLiteral("message"), d->errorMessage);
        detail::insertIfNotEmpty(error, QStringLiteral("param"), d->errorParam);
        json.insert(QStringLiteral("error"), error);
    }
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    return json;
}

FineTuningJob FineTuningJob::fromJson(const QJsonObject &json)
{
    FineTuningJob job;
    job.d->id = detail::stringOr(json, QStringLiteral("id"));
    job.d->object = detail::stringOr(json, QStringLiteral("object"));
    job.d->model = detail::stringOr(json, QStringLiteral("model"));
    job.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    job.d->finishedAt = detail::int64Or(json, QStringLiteral("finished_at"));
    job.d->fineTunedModel = detail::stringOr(json, QStringLiteral("fine_tuned_model"));
    job.d->organizationId = detail::stringOr(json, QStringLiteral("organization_id"));
    job.d->trainingFile = detail::stringOr(json, QStringLiteral("training_file"));
    job.d->validationFile = detail::stringOr(json, QStringLiteral("validation_file"));
    const QJsonArray resultFiles = json.value(QStringLiteral("result_files")).toArray();
    for (const QJsonValue &value : resultFiles)
        job.d->resultFiles.append(value.toString());
    job.d->status = fineTuningJobStatusFromString(detail::stringOr(json, QStringLiteral("status")));
    job.d->trainedTokens = detail::int64Or(json, QStringLiteral("trained_tokens"));
    job.d->seed = json.value(QStringLiteral("seed")).toInt();
    const QJsonObject method = json.value(QStringLiteral("method")).toObject();
    job.d->methodType = detail::stringOr(method, QStringLiteral("type"));
    job.d->hyperparameters = FineTuningHyperparameters::fromMethodJson(method);
    const QJsonObject error = json.value(QStringLiteral("error")).toObject();
    job.d->errorCode = detail::stringOr(error, QStringLiteral("code"));
    job.d->errorMessage = detail::stringOr(error, QStringLiteral("message"));
    job.d->errorParam = detail::stringOr(error, QStringLiteral("param"));
    job.d->metadata = json.value(QStringLiteral("metadata")).toObject();
    return job;
}

bool FineTuningJob::operator==(const FineTuningJob &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->model == other.d->model
           && d->createdAt == other.d->createdAt && d->finishedAt == other.d->finishedAt
           && d->fineTunedModel == other.d->fineTunedModel
           && d->organizationId == other.d->organizationId
           && d->trainingFile == other.d->trainingFile
           && d->validationFile == other.d->validationFile && d->resultFiles == other.d->resultFiles
           && d->status == other.d->status && d->trainedTokens == other.d->trainedTokens
           && d->seed == other.d->seed && d->methodType == other.d->methodType
           && d->hyperparameters == other.d->hyperparameters && d->errorCode == other.d->errorCode
           && d->errorMessage == other.d->errorMessage && d->errorParam == other.d->errorParam
           && d->metadata == other.d->metadata;
}

// --- FineTuningJobEvent ----------------------------------------------------

class FineTuningJobEventData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    QString level;
    QString message;
    QString type;
    QJsonObject data;
};

FineTuningJobEvent::FineTuningJobEvent()
    : d(new FineTuningJobEventData)
{ }

FineTuningJobEvent::FineTuningJobEvent(const FineTuningJobEvent &other) = default;
FineTuningJobEvent::FineTuningJobEvent(FineTuningJobEvent &&other) noexcept = default;
FineTuningJobEvent &FineTuningJobEvent::operator=(const FineTuningJobEvent &other) = default;
FineTuningJobEvent &FineTuningJobEvent::operator=(FineTuningJobEvent &&other) noexcept = default;
FineTuningJobEvent::~FineTuningJobEvent() = default;

QString FineTuningJobEvent::id() const { return d->id; }
void FineTuningJobEvent::setId(const QString &id) { d->id = id; }

QString FineTuningJobEvent::object() const { return d->object; }
void FineTuningJobEvent::setObject(const QString &object) { d->object = object; }

qint64 FineTuningJobEvent::createdAt() const { return d->createdAt; }
void FineTuningJobEvent::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString FineTuningJobEvent::level() const { return d->level; }
void FineTuningJobEvent::setLevel(const QString &level) { d->level = level; }

QString FineTuningJobEvent::message() const { return d->message; }
void FineTuningJobEvent::setMessage(const QString &message) { d->message = message; }

QString FineTuningJobEvent::type() const { return d->type; }
void FineTuningJobEvent::setType(const QString &type) { d->type = type; }

QJsonObject FineTuningJobEvent::data() const { return d->data; }
void FineTuningJobEvent::setData(const QJsonObject &data) { d->data = data; }

QJsonObject FineTuningJobEvent::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("level"), d->level);
    detail::insertIfNotEmpty(json, QStringLiteral("message"), d->message);
    detail::insertIfNotEmpty(json, QStringLiteral("type"), d->type);
    if (!d->data.isEmpty())
        json.insert(QStringLiteral("data"), d->data);
    return json;
}

FineTuningJobEvent FineTuningJobEvent::fromJson(const QJsonObject &json)
{
    FineTuningJobEvent event;
    event.d->id = detail::stringOr(json, QStringLiteral("id"));
    event.d->object = detail::stringOr(json, QStringLiteral("object"));
    event.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    event.d->level = detail::stringOr(json, QStringLiteral("level"));
    event.d->message = detail::stringOr(json, QStringLiteral("message"));
    event.d->type = detail::stringOr(json, QStringLiteral("type"));
    event.d->data = json.value(QStringLiteral("data")).toObject();
    return event;
}

bool FineTuningJobEvent::operator==(const FineTuningJobEvent &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->level == other.d->level
           && d->message == other.d->message && d->type == other.d->type
           && d->data == other.d->data;
}

} // namespace Core
} // namespace QtOpenAi
