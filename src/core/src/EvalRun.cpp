// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/EvalRun.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

// --- EvalResultCounts ------------------------------------------------------

QJsonObject EvalResultCounts::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("total"), total);
    json.insert(QStringLiteral("errored"), errored);
    json.insert(QStringLiteral("failed"), failed);
    json.insert(QStringLiteral("passed"), passed);
    return json;
}

EvalResultCounts EvalResultCounts::fromJson(const QJsonObject &json)
{
    EvalResultCounts counts;
    counts.total = json.value(QStringLiteral("total")).toInt();
    counts.errored = json.value(QStringLiteral("errored")).toInt();
    counts.failed = json.value(QStringLiteral("failed")).toInt();
    counts.passed = json.value(QStringLiteral("passed")).toInt();
    return counts;
}

// --- EvalRun ---------------------------------------------------------------

class EvalRunData : public QSharedData
{
public:
    QString id;
    QString object;
    QString evalId;
    QString name;
    qint64 createdAt = 0;
    EvalRunStatus status = EvalRunStatus::Queued;
    QString model;
    QString reportUrl;
    EvalResultCounts resultCounts;
    QJsonObject dataSource;
    QString errorCode;
    QString errorMessage;
    QJsonObject metadata;
};

EvalRun::EvalRun()
    : d(new EvalRunData)
{ }

EvalRun::EvalRun(const EvalRun &other) = default;
EvalRun::EvalRun(EvalRun &&other) noexcept = default;
EvalRun &EvalRun::operator=(const EvalRun &other) = default;
EvalRun &EvalRun::operator=(EvalRun &&other) noexcept = default;
EvalRun::~EvalRun() = default;

QString EvalRun::id() const { return d->id; }
void EvalRun::setId(const QString &id) { d->id = id; }

QString EvalRun::object() const { return d->object; }
void EvalRun::setObject(const QString &object) { d->object = object; }

QString EvalRun::evalId() const { return d->evalId; }
void EvalRun::setEvalId(const QString &evalId) { d->evalId = evalId; }

QString EvalRun::name() const { return d->name; }
void EvalRun::setName(const QString &name) { d->name = name; }

qint64 EvalRun::createdAt() const { return d->createdAt; }
void EvalRun::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

EvalRunStatus EvalRun::status() const { return d->status; }
void EvalRun::setStatus(EvalRunStatus status) { d->status = status; }

QString EvalRun::model() const { return d->model; }
void EvalRun::setModel(const QString &model) { d->model = model; }

QString EvalRun::reportUrl() const { return d->reportUrl; }
void EvalRun::setReportUrl(const QString &reportUrl) { d->reportUrl = reportUrl; }

EvalResultCounts EvalRun::resultCounts() const { return d->resultCounts; }
void EvalRun::setResultCounts(const EvalResultCounts &resultCounts)
{
    d->resultCounts = resultCounts;
}

QJsonObject EvalRun::dataSource() const { return d->dataSource; }
void EvalRun::setDataSource(const QJsonObject &dataSource) { d->dataSource = dataSource; }

QString EvalRun::errorCode() const { return d->errorCode; }
void EvalRun::setErrorCode(const QString &errorCode) { d->errorCode = errorCode; }

QString EvalRun::errorMessage() const { return d->errorMessage; }
void EvalRun::setErrorMessage(const QString &errorMessage) { d->errorMessage = errorMessage; }

QJsonObject EvalRun::metadata() const { return d->metadata; }
void EvalRun::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

bool EvalRun::isTerminal() const
{
    switch (d->status) {
    case EvalRunStatus::Completed:
    case EvalRunStatus::Failed:
    case EvalRunStatus::Canceled:
        return true;
    case EvalRunStatus::Queued:
    case EvalRunStatus::InProgress:
        return false;
    }
    return false;
}

QJsonObject EvalRun::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("eval_id"), d->evalId);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    json.insert(QStringLiteral("status"), evalRunStatusToString(d->status));
    detail::insertIfNotEmpty(json, QStringLiteral("model"), d->model);
    detail::insertIfNotEmpty(json, QStringLiteral("report_url"), d->reportUrl);
    json.insert(QStringLiteral("result_counts"), d->resultCounts.toJson());
    if (!d->dataSource.isEmpty())
        json.insert(QStringLiteral("data_source"), d->dataSource);
    if (!d->errorCode.isEmpty() || !d->errorMessage.isEmpty()) {
        QJsonObject error;
        detail::insertIfNotEmpty(error, QStringLiteral("code"), d->errorCode);
        detail::insertIfNotEmpty(error, QStringLiteral("message"), d->errorMessage);
        json.insert(QStringLiteral("error"), error);
    }
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    return json;
}

EvalRun EvalRun::fromJson(const QJsonObject &json)
{
    EvalRun run;
    // The deletion acknowledgement names the id `run_id`; accept both.
    run.d->id = detail::stringOr(json, QStringLiteral("id"),
                                 detail::stringOr(json, QStringLiteral("run_id")));
    run.d->object = detail::stringOr(json, QStringLiteral("object"));
    run.d->evalId = detail::stringOr(json, QStringLiteral("eval_id"));
    run.d->name = detail::stringOr(json, QStringLiteral("name"));
    run.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    run.d->status = evalRunStatusFromString(detail::stringOr(json, QStringLiteral("status")));
    run.d->model = detail::stringOr(json, QStringLiteral("model"));
    run.d->reportUrl = detail::stringOr(json, QStringLiteral("report_url"));
    run.d->resultCounts
            = EvalResultCounts::fromJson(json.value(QStringLiteral("result_counts")).toObject());
    run.d->dataSource = json.value(QStringLiteral("data_source")).toObject();
    const QJsonObject error = json.value(QStringLiteral("error")).toObject();
    run.d->errorCode = detail::stringOr(error, QStringLiteral("code"));
    run.d->errorMessage = detail::stringOr(error, QStringLiteral("message"));
    run.d->metadata = json.value(QStringLiteral("metadata")).toObject();
    return run;
}

bool EvalRun::operator==(const EvalRun &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->evalId == other.d->evalId
           && d->name == other.d->name && d->createdAt == other.d->createdAt
           && d->status == other.d->status && d->model == other.d->model
           && d->reportUrl == other.d->reportUrl && d->resultCounts == other.d->resultCounts
           && d->dataSource == other.d->dataSource && d->errorCode == other.d->errorCode
           && d->errorMessage == other.d->errorMessage && d->metadata == other.d->metadata;
}

// --- EvalRunOutputItem -----------------------------------------------------

class EvalRunOutputItemData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    QString runId;
    QString evalId;
    QString status;
    int datasourceItemId = 0;
    QJsonObject datasourceItem;
    QJsonArray results;
    QJsonObject sample;
};

EvalRunOutputItem::EvalRunOutputItem()
    : d(new EvalRunOutputItemData)
{ }

EvalRunOutputItem::EvalRunOutputItem(const EvalRunOutputItem &other) = default;
EvalRunOutputItem::EvalRunOutputItem(EvalRunOutputItem &&other) noexcept = default;
EvalRunOutputItem &EvalRunOutputItem::operator=(const EvalRunOutputItem &other) = default;
EvalRunOutputItem &EvalRunOutputItem::operator=(EvalRunOutputItem &&other) noexcept = default;
EvalRunOutputItem::~EvalRunOutputItem() = default;

QString EvalRunOutputItem::id() const { return d->id; }
void EvalRunOutputItem::setId(const QString &id) { d->id = id; }

QString EvalRunOutputItem::object() const { return d->object; }
void EvalRunOutputItem::setObject(const QString &object) { d->object = object; }

qint64 EvalRunOutputItem::createdAt() const { return d->createdAt; }
void EvalRunOutputItem::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString EvalRunOutputItem::runId() const { return d->runId; }
void EvalRunOutputItem::setRunId(const QString &runId) { d->runId = runId; }

QString EvalRunOutputItem::evalId() const { return d->evalId; }
void EvalRunOutputItem::setEvalId(const QString &evalId) { d->evalId = evalId; }

QString EvalRunOutputItem::status() const { return d->status; }
void EvalRunOutputItem::setStatus(const QString &status) { d->status = status; }

int EvalRunOutputItem::datasourceItemId() const { return d->datasourceItemId; }
void EvalRunOutputItem::setDatasourceItemId(int datasourceItemId)
{
    d->datasourceItemId = datasourceItemId;
}

QJsonObject EvalRunOutputItem::datasourceItem() const { return d->datasourceItem; }
void EvalRunOutputItem::setDatasourceItem(const QJsonObject &datasourceItem)
{
    d->datasourceItem = datasourceItem;
}

QJsonArray EvalRunOutputItem::results() const { return d->results; }
void EvalRunOutputItem::setResults(const QJsonArray &results) { d->results = results; }

QJsonObject EvalRunOutputItem::sample() const { return d->sample; }
void EvalRunOutputItem::setSample(const QJsonObject &sample) { d->sample = sample; }

QJsonObject EvalRunOutputItem::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("run_id"), d->runId);
    detail::insertIfNotEmpty(json, QStringLiteral("eval_id"), d->evalId);
    detail::insertIfNotEmpty(json, QStringLiteral("status"), d->status);
    if (d->datasourceItemId != 0)
        json.insert(QStringLiteral("datasource_item_id"), d->datasourceItemId);
    if (!d->datasourceItem.isEmpty())
        json.insert(QStringLiteral("datasource_item"), d->datasourceItem);
    if (!d->results.isEmpty())
        json.insert(QStringLiteral("results"), d->results);
    if (!d->sample.isEmpty())
        json.insert(QStringLiteral("sample"), d->sample);
    return json;
}

EvalRunOutputItem EvalRunOutputItem::fromJson(const QJsonObject &json)
{
    EvalRunOutputItem item;
    item.d->id = detail::stringOr(json, QStringLiteral("id"));
    item.d->object = detail::stringOr(json, QStringLiteral("object"));
    item.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    item.d->runId = detail::stringOr(json, QStringLiteral("run_id"));
    item.d->evalId = detail::stringOr(json, QStringLiteral("eval_id"));
    item.d->status = detail::stringOr(json, QStringLiteral("status"));
    item.d->datasourceItemId = json.value(QStringLiteral("datasource_item_id")).toInt();
    item.d->datasourceItem = json.value(QStringLiteral("datasource_item")).toObject();
    item.d->results = json.value(QStringLiteral("results")).toArray();
    item.d->sample = json.value(QStringLiteral("sample")).toObject();
    return item;
}

bool EvalRunOutputItem::operator==(const EvalRunOutputItem &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->runId == other.d->runId
           && d->evalId == other.d->evalId && d->status == other.d->status
           && d->datasourceItemId == other.d->datasourceItemId
           && d->datasourceItem == other.d->datasourceItem && d->results == other.d->results
           && d->sample == other.d->sample;
}

} // namespace Core
} // namespace QtOpenAi
