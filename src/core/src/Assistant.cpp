// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Assistant.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class AssistantData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    QString name;
    QString description;
    QString model;
    QString instructions;
    QJsonArray tools;
    QJsonObject toolResources;
    QJsonObject metadata;
    std::optional<double> temperature;
    std::optional<double> topP;
    QJsonValue responseFormat = QJsonValue::Undefined;
};

Assistant::Assistant()
    : d(new AssistantData)
{ }

Assistant::Assistant(const Assistant &other) = default;
Assistant::Assistant(Assistant &&other) noexcept = default;
Assistant &Assistant::operator=(const Assistant &other) = default;
Assistant &Assistant::operator=(Assistant &&other) noexcept = default;
Assistant::~Assistant() = default;

QString Assistant::id() const { return d->id; }
void Assistant::setId(const QString &id) { d->id = id; }

QString Assistant::object() const { return d->object; }
void Assistant::setObject(const QString &object) { d->object = object; }

qint64 Assistant::createdAt() const { return d->createdAt; }
void Assistant::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString Assistant::name() const { return d->name; }
void Assistant::setName(const QString &name) { d->name = name; }

QString Assistant::description() const { return d->description; }
void Assistant::setDescription(const QString &description) { d->description = description; }

QString Assistant::model() const { return d->model; }
void Assistant::setModel(const QString &model) { d->model = model; }

QString Assistant::instructions() const { return d->instructions; }
void Assistant::setInstructions(const QString &instructions) { d->instructions = instructions; }

QJsonArray Assistant::tools() const { return d->tools; }
void Assistant::setTools(const QJsonArray &tools) { d->tools = tools; }

QJsonObject Assistant::toolResources() const { return d->toolResources; }
void Assistant::setToolResources(const QJsonObject &toolResources)
{
    d->toolResources = toolResources;
}

QJsonObject Assistant::metadata() const { return d->metadata; }
void Assistant::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

std::optional<double> Assistant::temperature() const { return d->temperature; }
void Assistant::setTemperature(double temperature) { d->temperature = temperature; }

std::optional<double> Assistant::topP() const { return d->topP; }
void Assistant::setTopP(double topP) { d->topP = topP; }

QJsonValue Assistant::responseFormat() const { return d->responseFormat; }
void Assistant::setResponseFormat(const QJsonValue &responseFormat)
{
    d->responseFormat = responseFormat;
}

QJsonObject Assistant::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNotEmpty(json, QStringLiteral("description"), d->description);
    detail::insertIfNotEmpty(json, QStringLiteral("model"), d->model);
    detail::insertIfNotEmpty(json, QStringLiteral("instructions"), d->instructions);
    if (!d->tools.isEmpty())
        json.insert(QStringLiteral("tools"), d->tools);
    if (!d->toolResources.isEmpty())
        json.insert(QStringLiteral("tool_resources"), d->toolResources);
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    detail::insertIfSet(json, QStringLiteral("temperature"), d->temperature);
    detail::insertIfSet(json, QStringLiteral("top_p"), d->topP);
    if (!d->responseFormat.isUndefined())
        json.insert(QStringLiteral("response_format"), d->responseFormat);
    return json;
}

Assistant Assistant::fromJson(const QJsonObject &json)
{
    Assistant assistant;
    assistant.d->id = detail::stringOr(json, QStringLiteral("id"));
    assistant.d->object = detail::stringOr(json, QStringLiteral("object"));
    assistant.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    assistant.d->name = detail::stringOr(json, QStringLiteral("name"));
    assistant.d->description = detail::stringOr(json, QStringLiteral("description"));
    assistant.d->model = detail::stringOr(json, QStringLiteral("model"));
    assistant.d->instructions = detail::stringOr(json, QStringLiteral("instructions"));
    assistant.d->tools = json.value(QStringLiteral("tools")).toArray();
    assistant.d->toolResources = json.value(QStringLiteral("tool_resources")).toObject();
    assistant.d->metadata = json.value(QStringLiteral("metadata")).toObject();
    assistant.d->temperature = detail::optionalDouble(json, QStringLiteral("temperature"));
    assistant.d->topP = detail::optionalDouble(json, QStringLiteral("top_p"));
    // A null response_format means "not set", the same as an absent one.
    const QJsonValue format = json.value(QStringLiteral("response_format"));
    assistant.d->responseFormat = format.isNull() ? QJsonValue(QJsonValue::Undefined) : format;
    return assistant;
}

bool Assistant::operator==(const Assistant &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->name == other.d->name
           && d->description == other.d->description && d->model == other.d->model
           && d->instructions == other.d->instructions && d->tools == other.d->tools
           && d->toolResources == other.d->toolResources && d->metadata == other.d->metadata
           && d->temperature == other.d->temperature && d->topP == other.d->topP
           && d->responseFormat == other.d->responseFormat;
}

} // namespace Core
} // namespace QtOpenAi
