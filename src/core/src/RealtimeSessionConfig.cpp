// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/RealtimeSessionConfig.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

namespace {

// Read a field that is only meaningful when the server actually sent it, so an
// absent one stays undefined rather than becoming a null the next
// `session.update` would ship back as a reset.
QJsonValue definedOr(const QJsonObject &json, const QString &key)
{
    const QJsonValue value = json.value(key);
    return value.isUndefined() || value.isNull() ? QJsonValue(QJsonValue::Undefined) : value;
}

void insertIfDefined(QJsonObject &json, const QString &key, const QJsonValue &value)
{
    if (!value.isUndefined())
        json.insert(key, value);
}

} // namespace

class RealtimeSessionConfigData : public QSharedData
{
public:
    QString type;
    QString id;
    QString object;
    QString model;
    QString instructions;
    QStringList outputModalities;
    QJsonObject audio;
    QJsonArray tools;
    QJsonValue toolChoice = QJsonValue::Undefined;
    QJsonValue maxOutputTokens = QJsonValue::Undefined;
    QJsonValue tracing = QJsonValue::Undefined;
    QStringList include;
    QJsonObject prompt;
    qint64 expiresAt = 0;
};

RealtimeSessionConfig::RealtimeSessionConfig()
    : d(new RealtimeSessionConfigData)
{ }

RealtimeSessionConfig::RealtimeSessionConfig(const RealtimeSessionConfig &other) = default;
RealtimeSessionConfig::RealtimeSessionConfig(RealtimeSessionConfig &&other) noexcept = default;
RealtimeSessionConfig &RealtimeSessionConfig::operator=(const RealtimeSessionConfig &other)
        = default;
RealtimeSessionConfig &RealtimeSessionConfig::operator=(RealtimeSessionConfig &&other) noexcept
        = default;
RealtimeSessionConfig::~RealtimeSessionConfig() = default;

QString RealtimeSessionConfig::type() const { return d->type; }
void RealtimeSessionConfig::setType(const QString &type) { d->type = type; }

QString RealtimeSessionConfig::id() const { return d->id; }
void RealtimeSessionConfig::setId(const QString &id) { d->id = id; }

QString RealtimeSessionConfig::object() const { return d->object; }
void RealtimeSessionConfig::setObject(const QString &object) { d->object = object; }

QString RealtimeSessionConfig::model() const { return d->model; }
void RealtimeSessionConfig::setModel(const QString &model) { d->model = model; }

QString RealtimeSessionConfig::instructions() const { return d->instructions; }
void RealtimeSessionConfig::setInstructions(const QString &instructions)
{
    d->instructions = instructions;
}

QStringList RealtimeSessionConfig::outputModalities() const { return d->outputModalities; }
void RealtimeSessionConfig::setOutputModalities(const QStringList &outputModalities)
{
    d->outputModalities = outputModalities;
}

QJsonObject RealtimeSessionConfig::audio() const { return d->audio; }
void RealtimeSessionConfig::setAudio(const QJsonObject &audio) { d->audio = audio; }

QJsonArray RealtimeSessionConfig::tools() const { return d->tools; }
void RealtimeSessionConfig::setTools(const QJsonArray &tools) { d->tools = tools; }

QJsonValue RealtimeSessionConfig::toolChoice() const { return d->toolChoice; }
void RealtimeSessionConfig::setToolChoice(const QJsonValue &toolChoice)
{
    d->toolChoice = toolChoice;
}

QJsonValue RealtimeSessionConfig::maxOutputTokens() const { return d->maxOutputTokens; }
void RealtimeSessionConfig::setMaxOutputTokens(const QJsonValue &maxOutputTokens)
{
    d->maxOutputTokens = maxOutputTokens;
}

QJsonValue RealtimeSessionConfig::tracing() const { return d->tracing; }
void RealtimeSessionConfig::setTracing(const QJsonValue &tracing) { d->tracing = tracing; }

QStringList RealtimeSessionConfig::include() const { return d->include; }
void RealtimeSessionConfig::setInclude(const QStringList &include) { d->include = include; }

QJsonObject RealtimeSessionConfig::prompt() const { return d->prompt; }
void RealtimeSessionConfig::setPrompt(const QJsonObject &prompt) { d->prompt = prompt; }

qint64 RealtimeSessionConfig::expiresAt() const { return d->expiresAt; }
void RealtimeSessionConfig::setExpiresAt(qint64 expiresAt) { d->expiresAt = expiresAt; }

QJsonObject RealtimeSessionConfig::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("type"), d->type);
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("model"), d->model);
    detail::insertIfNotEmpty(json, QStringLiteral("instructions"), d->instructions);
    detail::insertIfNotEmpty(json, QStringLiteral("output_modalities"), d->outputModalities);
    if (!d->audio.isEmpty())
        json.insert(QStringLiteral("audio"), d->audio);
    if (!d->tools.isEmpty())
        json.insert(QStringLiteral("tools"), d->tools);
    insertIfDefined(json, QStringLiteral("tool_choice"), d->toolChoice);
    insertIfDefined(json, QStringLiteral("max_output_tokens"), d->maxOutputTokens);
    insertIfDefined(json, QStringLiteral("tracing"), d->tracing);
    detail::insertIfNotEmpty(json, QStringLiteral("include"), d->include);
    if (!d->prompt.isEmpty())
        json.insert(QStringLiteral("prompt"), d->prompt);
    detail::insertIfNonZero(json, QStringLiteral("expires_at"), d->expiresAt);
    return json;
}

RealtimeSessionConfig RealtimeSessionConfig::fromJson(const QJsonObject &json)
{
    RealtimeSessionConfig config;
    config.d->type = detail::stringOr(json, QStringLiteral("type"));
    config.d->id = detail::stringOr(json, QStringLiteral("id"));
    config.d->object = detail::stringOr(json, QStringLiteral("object"));
    config.d->model = detail::stringOr(json, QStringLiteral("model"));
    config.d->instructions = detail::stringOr(json, QStringLiteral("instructions"));
    config.d->outputModalities = detail::stringListOr(json, QStringLiteral("output_modalities"));
    config.d->audio = json.value(QStringLiteral("audio")).toObject();
    config.d->tools = json.value(QStringLiteral("tools")).toArray();
    config.d->toolChoice = definedOr(json, QStringLiteral("tool_choice"));
    config.d->maxOutputTokens = definedOr(json, QStringLiteral("max_output_tokens"));
    config.d->tracing = definedOr(json, QStringLiteral("tracing"));
    config.d->include = detail::stringListOr(json, QStringLiteral("include"));
    config.d->prompt = json.value(QStringLiteral("prompt")).toObject();
    config.d->expiresAt = detail::int64Or(json, QStringLiteral("expires_at"));
    return config;
}

bool RealtimeSessionConfig::operator==(const RealtimeSessionConfig &other) const
{
    return d->type == other.d->type && d->id == other.d->id && d->object == other.d->object
           && d->model == other.d->model && d->instructions == other.d->instructions
           && d->outputModalities == other.d->outputModalities && d->audio == other.d->audio
           && d->tools == other.d->tools && d->toolChoice == other.d->toolChoice
           && d->maxOutputTokens == other.d->maxOutputTokens && d->tracing == other.d->tracing
           && d->include == other.d->include && d->prompt == other.d->prompt
           && d->expiresAt == other.d->expiresAt;
}

} // namespace Core
} // namespace QtOpenAi
