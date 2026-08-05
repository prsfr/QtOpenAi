// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ProjectRateLimit.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

namespace {

// The wire names, spelled once each because both directions need them.
constexpr auto kMaxRequestsPerMinute = "max_requests_per_1_minute";
constexpr auto kMaxTokensPerMinute = "max_tokens_per_1_minute";
constexpr auto kMaxImagesPerMinute = "max_images_per_1_minute";
constexpr auto kMaxAudioMegabytesPerMinute = "max_audio_megabytes_per_1_minute";
constexpr auto kMaxRequestsPerDay = "max_requests_per_1_day";
constexpr auto kBatchMaxInputTokensPerDay = "batch_1_day_max_input_tokens";

// Read a limit the server may not report at all. Not int64Or(): a 0 fallback
// would be a limit of zero, which is a real and very different setting.
std::optional<qint64> optionalInt64(const QJsonObject &json, const char *key)
{
    const QJsonValue value = json.value(QLatin1String(key));
    return value.isDouble() ? std::optional<qint64>(value.toVariant().toLongLong()) : std::nullopt;
}

void insertIfSet(QJsonObject &json, const char *key, const std::optional<qint64> &value)
{
    if (value)
        json.insert(QLatin1String(key), *value);
}

} // namespace

class ProjectRateLimitData : public QSharedData
{
public:
    QString id;
    QString object;
    QString model;
    std::optional<qint64> maxRequestsPerMinute;
    std::optional<qint64> maxTokensPerMinute;
    std::optional<qint64> maxImagesPerMinute;
    std::optional<qint64> maxAudioMegabytesPerMinute;
    std::optional<qint64> maxRequestsPerDay;
    std::optional<qint64> batchMaxInputTokensPerDay;
};

ProjectRateLimit::ProjectRateLimit()
    : d(new ProjectRateLimitData)
{ }

ProjectRateLimit::ProjectRateLimit(const ProjectRateLimit &other) = default;
ProjectRateLimit::ProjectRateLimit(ProjectRateLimit &&other) noexcept = default;
ProjectRateLimit &ProjectRateLimit::operator=(const ProjectRateLimit &other) = default;
ProjectRateLimit &ProjectRateLimit::operator=(ProjectRateLimit &&other) noexcept = default;
ProjectRateLimit::~ProjectRateLimit() = default;

QString ProjectRateLimit::id() const { return d->id; }
void ProjectRateLimit::setId(const QString &id) { d->id = id; }

QString ProjectRateLimit::object() const { return d->object; }
void ProjectRateLimit::setObject(const QString &object) { d->object = object; }

QString ProjectRateLimit::model() const { return d->model; }
void ProjectRateLimit::setModel(const QString &model) { d->model = model; }

std::optional<qint64> ProjectRateLimit::maxRequestsPerMinute() const
{
    return d->maxRequestsPerMinute;
}
void ProjectRateLimit::setMaxRequestsPerMinute(std::optional<qint64> value)
{
    d->maxRequestsPerMinute = value;
}

std::optional<qint64> ProjectRateLimit::maxTokensPerMinute() const { return d->maxTokensPerMinute; }
void ProjectRateLimit::setMaxTokensPerMinute(std::optional<qint64> value)
{
    d->maxTokensPerMinute = value;
}

std::optional<qint64> ProjectRateLimit::maxImagesPerMinute() const { return d->maxImagesPerMinute; }
void ProjectRateLimit::setMaxImagesPerMinute(std::optional<qint64> value)
{
    d->maxImagesPerMinute = value;
}

std::optional<qint64> ProjectRateLimit::maxAudioMegabytesPerMinute() const
{
    return d->maxAudioMegabytesPerMinute;
}
void ProjectRateLimit::setMaxAudioMegabytesPerMinute(std::optional<qint64> value)
{
    d->maxAudioMegabytesPerMinute = value;
}

std::optional<qint64> ProjectRateLimit::maxRequestsPerDay() const { return d->maxRequestsPerDay; }
void ProjectRateLimit::setMaxRequestsPerDay(std::optional<qint64> value)
{
    d->maxRequestsPerDay = value;
}

std::optional<qint64> ProjectRateLimit::batchMaxInputTokensPerDay() const
{
    return d->batchMaxInputTokensPerDay;
}
void ProjectRateLimit::setBatchMaxInputTokensPerDay(std::optional<qint64> value)
{
    d->batchMaxInputTokensPerDay = value;
}

bool ProjectRateLimit::isEmpty() const
{
    return !d->maxRequestsPerMinute && !d->maxTokensPerMinute && !d->maxImagesPerMinute
           && !d->maxAudioMegabytesPerMinute && !d->maxRequestsPerDay
           && !d->batchMaxInputTokensPerDay;
}

QJsonObject ProjectRateLimit::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("model"), d->model);
    insertIfSet(json, kMaxRequestsPerMinute, d->maxRequestsPerMinute);
    insertIfSet(json, kMaxTokensPerMinute, d->maxTokensPerMinute);
    insertIfSet(json, kMaxImagesPerMinute, d->maxImagesPerMinute);
    insertIfSet(json, kMaxAudioMegabytesPerMinute, d->maxAudioMegabytesPerMinute);
    insertIfSet(json, kMaxRequestsPerDay, d->maxRequestsPerDay);
    insertIfSet(json, kBatchMaxInputTokensPerDay, d->batchMaxInputTokensPerDay);
    return json;
}

ProjectRateLimit ProjectRateLimit::fromJson(const QJsonObject &json)
{
    ProjectRateLimit limit;
    limit.d->id = detail::stringOr(json, QStringLiteral("id"));
    limit.d->object = detail::stringOr(json, QStringLiteral("object"));
    limit.d->model = detail::stringOr(json, QStringLiteral("model"));
    limit.d->maxRequestsPerMinute = optionalInt64(json, kMaxRequestsPerMinute);
    limit.d->maxTokensPerMinute = optionalInt64(json, kMaxTokensPerMinute);
    limit.d->maxImagesPerMinute = optionalInt64(json, kMaxImagesPerMinute);
    limit.d->maxAudioMegabytesPerMinute = optionalInt64(json, kMaxAudioMegabytesPerMinute);
    limit.d->maxRequestsPerDay = optionalInt64(json, kMaxRequestsPerDay);
    limit.d->batchMaxInputTokensPerDay = optionalInt64(json, kBatchMaxInputTokensPerDay);
    return limit;
}

bool ProjectRateLimit::operator==(const ProjectRateLimit &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->model == other.d->model
           && d->maxRequestsPerMinute == other.d->maxRequestsPerMinute
           && d->maxTokensPerMinute == other.d->maxTokensPerMinute
           && d->maxImagesPerMinute == other.d->maxImagesPerMinute
           && d->maxAudioMegabytesPerMinute == other.d->maxAudioMegabytesPerMinute
           && d->maxRequestsPerDay == other.d->maxRequestsPerDay
           && d->batchMaxInputTokensPerDay == other.d->batchMaxInputTokensPerDay;
}

} // namespace Core
} // namespace QtOpenAi
