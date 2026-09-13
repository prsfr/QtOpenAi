// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Assistant.h"

#include "AssistantFields_p.h"
#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

// The ten fields an assistant shares with the body that creates one come from
// the private base; only what the server stamps on it lives here. See
// AssistantFields_p.h for why they are not declared twice.
class AssistantData : public detail::AssistantFieldsData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
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
    detail::insertAssistantFields(json, *d);
    return json;
}

Assistant Assistant::fromJson(const QJsonObject &json)
{
    Assistant assistant;
    assistant.d->id = detail::stringOr(json, QStringLiteral("id"));
    assistant.d->object = detail::stringOr(json, QStringLiteral("object"));
    assistant.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    detail::readAssistantFields(*assistant.d, json);
    return assistant;
}

bool Assistant::operator==(const Assistant &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && detail::assistantFieldsEqual(*d, *other.d);
}

} // namespace Core
} // namespace QtOpenAi
