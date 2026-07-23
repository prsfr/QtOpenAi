// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/CompletionResponse.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class CompletionResponseData : public QSharedData
{
public:
    QString id;
    QString object = QStringLiteral("text_completion");
    qint64 created = 0;
    QString model;
    QList<CompletionChoice> choices;
    Usage usage;
};

CompletionResponse::CompletionResponse()
    : d(new CompletionResponseData)
{ }

CompletionResponse::CompletionResponse(const CompletionResponse &other) = default;
CompletionResponse::CompletionResponse(CompletionResponse &&other) noexcept = default;
CompletionResponse &CompletionResponse::operator=(const CompletionResponse &other) = default;
CompletionResponse &CompletionResponse::operator=(CompletionResponse &&other) noexcept = default;
CompletionResponse::~CompletionResponse() = default;

QString CompletionResponse::id() const { return d->id; }
void CompletionResponse::setId(const QString &id) { d->id = id; }

QString CompletionResponse::object() const { return d->object; }
void CompletionResponse::setObject(const QString &object) { d->object = object; }

qint64 CompletionResponse::created() const { return d->created; }
void CompletionResponse::setCreated(qint64 created) { d->created = created; }

QString CompletionResponse::model() const { return d->model; }
void CompletionResponse::setModel(const QString &model) { d->model = model; }

QList<CompletionChoice> CompletionResponse::choices() const { return d->choices; }
void CompletionResponse::setChoices(const QList<CompletionChoice> &choices)
{
    d->choices = choices;
}

Usage CompletionResponse::usage() const { return d->usage; }
void CompletionResponse::setUsage(const Usage &usage) { d->usage = usage; }

QString CompletionResponse::firstText() const
{
    return d->choices.isEmpty() ? QString() : d->choices.first().text();
}

QJsonObject CompletionResponse::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("id"), d->id);
    json.insert(QStringLiteral("object"), d->object);
    json.insert(QStringLiteral("created"), d->created);
    json.insert(QStringLiteral("model"), d->model);

    QJsonArray choices;
    for (const CompletionChoice &choice : d->choices)
        choices.append(choice.toJson());
    json.insert(QStringLiteral("choices"), choices);

    json.insert(QStringLiteral("usage"), d->usage.toJson());
    return json;
}

CompletionResponse CompletionResponse::fromJson(const QJsonObject &json)
{
    CompletionResponse response;
    response.d->id = detail::stringOr(json, QStringLiteral("id"));
    response.d->object
            = detail::stringOr(json, QStringLiteral("object"), QStringLiteral("text_completion"));
    response.d->created = static_cast<qint64>(json.value(QStringLiteral("created")).toDouble());
    response.d->model = detail::stringOr(json, QStringLiteral("model"));

    const QJsonArray choices = json.value(QStringLiteral("choices")).toArray();
    for (const QJsonValue &value : choices)
        response.d->choices.append(CompletionChoice::fromJson(value.toObject()));

    if (json.contains(QStringLiteral("usage")) && json.value(QStringLiteral("usage")).isObject())
        response.d->usage = Usage::fromJson(json.value(QStringLiteral("usage")).toObject());

    return response;
}

bool CompletionResponse::operator==(const CompletionResponse &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->created == other.d->created
           && d->model == other.d->model && d->choices == other.d->choices
           && d->usage == other.d->usage;
}

} // namespace Core
} // namespace QtOpenAi
