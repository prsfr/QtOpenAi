// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/OrganizationUsage.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

namespace {

// The keys a usage row carries that are *not* counters. Everything else with a
// numeric value is one — that is how a counter this build has never heard of
// still survives a round trip. Kept next to fromJson() because the two only make
// sense together.
bool isGroupingKey(const QString &key)
{
    return key == QLatin1String("object") || key == QLatin1String("project_id")
           || key == QLatin1String("user_id") || key == QLatin1String("api_key_id")
           || key == QLatin1String("model") || key == QLatin1String("source")
           || key == QLatin1String("size") || key == QLatin1String("batch");
}

} // namespace

class UsageResultData : public QSharedData
{
public:
    QString object;
    QString projectId;
    QString userId;
    QString apiKeyId;
    QString model;
    QString source;
    QString size;
    std::optional<bool> batch;
    QMap<QString, qint64> metrics;
};

UsageResult::UsageResult()
    : d(new UsageResultData)
{ }

UsageResult::UsageResult(const UsageResult &other) = default;
UsageResult::UsageResult(UsageResult &&other) noexcept = default;
UsageResult &UsageResult::operator=(const UsageResult &other) = default;
UsageResult &UsageResult::operator=(UsageResult &&other) noexcept = default;
UsageResult::~UsageResult() = default;

QString UsageResult::object() const { return d->object; }
void UsageResult::setObject(const QString &object) { d->object = object; }

QString UsageResult::projectId() const { return d->projectId; }
void UsageResult::setProjectId(const QString &projectId) { d->projectId = projectId; }

QString UsageResult::userId() const { return d->userId; }
void UsageResult::setUserId(const QString &userId) { d->userId = userId; }

QString UsageResult::apiKeyId() const { return d->apiKeyId; }
void UsageResult::setApiKeyId(const QString &apiKeyId) { d->apiKeyId = apiKeyId; }

QString UsageResult::model() const { return d->model; }
void UsageResult::setModel(const QString &model) { d->model = model; }

QString UsageResult::source() const { return d->source; }
void UsageResult::setSource(const QString &source) { d->source = source; }

QString UsageResult::size() const { return d->size; }
void UsageResult::setSize(const QString &size) { d->size = size; }

std::optional<bool> UsageResult::batch() const { return d->batch; }
void UsageResult::setBatch(std::optional<bool> batch) { d->batch = batch; }

qint64 UsageResult::metric(const QString &name) const { return d->metrics.value(name, 0); }
void UsageResult::setMetric(const QString &name, qint64 value) { d->metrics.insert(name, value); }

QMap<QString, qint64> UsageResult::metrics() const { return d->metrics; }
void UsageResult::setMetrics(const QMap<QString, qint64> &metrics) { d->metrics = metrics; }

QJsonObject UsageResult::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    // Left out rather than written as null: the API's null means "this row is
    // not grouped by that key", which is exactly what an absent key says.
    detail::insertIfNotEmpty(json, QStringLiteral("project_id"), d->projectId);
    detail::insertIfNotEmpty(json, QStringLiteral("user_id"), d->userId);
    detail::insertIfNotEmpty(json, QStringLiteral("api_key_id"), d->apiKeyId);
    detail::insertIfNotEmpty(json, QStringLiteral("model"), d->model);
    detail::insertIfNotEmpty(json, QStringLiteral("source"), d->source);
    detail::insertIfNotEmpty(json, QStringLiteral("size"), d->size);
    detail::insertIfSet(json, QStringLiteral("batch"), d->batch);

    // Counters go out with a value even when it is 0: a row that reports a
    // counter at all reports it, and dropping the zero would turn "nothing
    // happened" into "this endpoint has no such counter".
    for (auto it = d->metrics.constBegin(); it != d->metrics.constEnd(); ++it)
        json.insert(it.key(), it.value());
    return json;
}

UsageResult UsageResult::fromJson(const QJsonObject &json)
{
    UsageResult result;
    result.d->object = detail::stringOr(json, QStringLiteral("object"));
    result.d->projectId = detail::stringOr(json, QStringLiteral("project_id"));
    result.d->userId = detail::stringOr(json, QStringLiteral("user_id"));
    result.d->apiKeyId = detail::stringOr(json, QStringLiteral("api_key_id"));
    result.d->model = detail::stringOr(json, QStringLiteral("model"));
    result.d->source = detail::stringOr(json, QStringLiteral("source"));
    result.d->size = detail::stringOr(json, QStringLiteral("size"));
    result.d->batch = detail::optionalBool(json, QStringLiteral("batch"));

    for (auto it = json.constBegin(); it != json.constEnd(); ++it) {
        if (!isGroupingKey(it.key()) && it.value().isDouble())
            result.d->metrics.insert(it.key(), it.value().toVariant().toLongLong());
    }
    return result;
}

bool UsageResult::operator==(const UsageResult &other) const
{
    return d->object == other.d->object && d->projectId == other.d->projectId
           && d->userId == other.d->userId && d->apiKeyId == other.d->apiKeyId
           && d->model == other.d->model && d->source == other.d->source && d->size == other.d->size
           && d->batch == other.d->batch && d->metrics == other.d->metrics;
}

} // namespace Core
} // namespace QtOpenAi
