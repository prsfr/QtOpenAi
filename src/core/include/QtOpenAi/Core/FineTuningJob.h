// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/Enums.h>
#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <optional>

namespace QtOpenAi {
namespace Core {

// The training hyperparameters of a fine-tuning job.
//
// Each one may be left to the service, which sends the string "auto" on the
// wire; that maps to an unset optional here rather than a made-up number, and an
// unset value is omitted from a request body so the service keeps choosing.
struct QTOPENAI_CORE_EXPORT FineTuningHyperparameters
{
    std::optional<int> nEpochs;
    std::optional<int> batchSize;
    std::optional<double> learningRateMultiplier;

    QJsonObject toJson() const;
    static FineTuningHyperparameters fromJson(const QJsonObject &json);

    // The wire format nests these one level below the method's own type:
    //   method: { type: "supervised", supervised: { hyperparameters: {...} } }
    // Both the job and the create-request go through these two helpers so that
    // nesting is spelled out in exactly one place.
    QJsonObject toMethodJson(const QString &methodType) const;
    static FineTuningHyperparameters fromMethodJson(const QJsonObject &method);

    bool operator==(const FineTuningHyperparameters &other) const
    {
        return nEpochs == other.nEpochs && batchSize == other.batchSize
               && learningRateMultiplier == other.learningRateMultiplier;
    }
    bool operator!=(const FineTuningHyperparameters &other) const { return !(*this == other); }
};

class FineTuningJobData;

// One fine-tuning job (POST /fine_tuning/jobs, GET /fine_tuning/jobs/{id}, ...).
//
// Training runs asynchronously and can take hours: a created job starts in
// `validating_files` and the client polls GET /fine_tuning/jobs/{id} (or uses
// Client::pollFineTuningJob()) until it becomes terminal. On success
// fineTunedModel() names the model to use in subsequent requests.
class QTOPENAI_CORE_EXPORT FineTuningJob
{
public:
    FineTuningJob();
    FineTuningJob(const FineTuningJob &other);
    FineTuningJob(FineTuningJob &&other) noexcept;
    FineTuningJob &operator=(const FineTuningJob &other);
    FineTuningJob &operator=(FineTuningJob &&other) noexcept;
    ~FineTuningJob();

    void swap(FineTuningJob &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "fine_tuning.job".
    QString object() const;
    void setObject(const QString &object);

    // The base model being fine-tuned.
    QString model() const;
    void setModel(const QString &model);

    // Unix creation timestamp (`created_at`); 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    // Unix completion timestamp (`finished_at`); 0 while still training.
    qint64 finishedAt() const;
    void setFinishedAt(qint64 finishedAt);

    // Name of the resulting model; empty until the job succeeds.
    QString fineTunedModel() const;
    void setFineTunedModel(const QString &fineTunedModel);

    QString organizationId() const;
    void setOrganizationId(const QString &organizationId);

    // Files API id of the training data (JSONL, purpose "fine-tune").
    QString trainingFile() const;
    void setTrainingFile(const QString &trainingFile);

    // Optional validation data; empty when the job has none.
    QString validationFile() const;
    void setValidationFile(const QString &validationFile);

    // Files API ids of the generated result files (metrics), once available.
    QStringList resultFiles() const;
    void setResultFiles(const QStringList &resultFiles);

    FineTuningJobStatus status() const;
    void setStatus(FineTuningJobStatus status);

    // Billed training tokens; 0 until the job finishes.
    qint64 trainedTokens() const;
    void setTrainedTokens(qint64 trainedTokens);

    // The seed used for reproducibility; 0 when absent.
    int seed() const;
    void setSeed(int seed);

    // The training method, e.g. "supervised", "dpo" or "reinforcement". The
    // hyperparameters below are the ones nested under it.
    QString methodType() const;
    void setMethodType(const QString &methodType);

    FineTuningHyperparameters hyperparameters() const;
    void setHyperparameters(const FineTuningHyperparameters &hyperparameters);

    // The failure code/message/param from the `error` object (populated when the
    // job status is Failed); all empty otherwise.
    QString errorCode() const;
    void setErrorCode(const QString &errorCode);

    QString errorMessage() const;
    void setErrorMessage(const QString &errorMessage);

    QString errorParam() const;
    void setErrorParam(const QString &errorParam);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    // True once the job has reached a state it will no longer leave (Succeeded,
    // Failed or Cancelled); polling can stop. A Paused job is *not* terminal —
    // it resumes on request.
    bool isTerminal() const;

    QJsonObject toJson() const;
    static FineTuningJob fromJson(const QJsonObject &json);

    bool operator==(const FineTuningJob &other) const;
    bool operator!=(const FineTuningJob &other) const { return !(*this == other); }

private:
    QSharedDataPointer<FineTuningJobData> d;
};

// A `list` of fine-tuning jobs (GET /fine_tuning/jobs).
using FineTuningJobList = ListPage<FineTuningJob>;

class FineTuningJobEventData;

// One entry of a job's event log (GET /fine_tuning/jobs/{id}/events) — either a
// human-readable progress message or a metrics sample, which `type` tells apart.
class QTOPENAI_CORE_EXPORT FineTuningJobEvent
{
public:
    FineTuningJobEvent();
    FineTuningJobEvent(const FineTuningJobEvent &other);
    FineTuningJobEvent(FineTuningJobEvent &&other) noexcept;
    FineTuningJobEvent &operator=(const FineTuningJobEvent &other);
    FineTuningJobEvent &operator=(FineTuningJobEvent &&other) noexcept;
    ~FineTuningJobEvent();

    void swap(FineTuningJobEvent &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "fine_tuning.job.event".
    QString object() const;
    void setObject(const QString &object);

    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    // Log level, e.g. "info", "warn" or "error". Kept as a string: it is a
    // pass-through log field, not something the library branches on.
    QString level() const;
    void setLevel(const QString &level);

    QString message() const;
    void setMessage(const QString &message);

    // "message" or "metrics".
    QString type() const;
    void setType(const QString &type);

    // The structured payload of a metrics event; empty for plain messages.
    QJsonObject data() const;
    void setData(const QJsonObject &data);

    QJsonObject toJson() const;
    static FineTuningJobEvent fromJson(const QJsonObject &json);

    bool operator==(const FineTuningJobEvent &other) const;
    bool operator!=(const FineTuningJobEvent &other) const { return !(*this == other); }

private:
    QSharedDataPointer<FineTuningJobEventData> d;
};

// A `list` of job events (GET /fine_tuning/jobs/{id}/events).
using FineTuningJobEventList = ListPage<FineTuningJobEvent>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::FineTuningJob)
Q_DECLARE_SHARED(QtOpenAi::Core::FineTuningJobEvent)
Q_DECLARE_METATYPE(QtOpenAi::Core::FineTuningJob)
Q_DECLARE_METATYPE(QtOpenAi::Core::FineTuningJobList)
Q_DECLARE_METATYPE(QtOpenAi::Core::FineTuningJobEvent)
Q_DECLARE_METATYPE(QtOpenAi::Core::FineTuningJobEventList)
