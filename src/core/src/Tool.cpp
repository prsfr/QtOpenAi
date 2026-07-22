// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Tool.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

// ---------------------------------------------------------------------------
// FunctionDefinition
// ---------------------------------------------------------------------------

class FunctionDefinitionData : public QSharedData
{
public:
    QString name;
    QString description;
    QJsonObject parameters;
};

FunctionDefinition::FunctionDefinition()
    : d(new FunctionDefinitionData)
{
}

FunctionDefinition::FunctionDefinition(QString name, QString description, QJsonObject parameters)
    : d(new FunctionDefinitionData)
{
    d->name = std::move(name);
    d->description = std::move(description);
    d->parameters = std::move(parameters);
}

FunctionDefinition::FunctionDefinition(const FunctionDefinition &other) = default;
FunctionDefinition::FunctionDefinition(FunctionDefinition &&other) noexcept = default;
FunctionDefinition &FunctionDefinition::operator=(const FunctionDefinition &other) = default;
FunctionDefinition &FunctionDefinition::operator=(FunctionDefinition &&other) noexcept = default;
FunctionDefinition::~FunctionDefinition() = default;

QString FunctionDefinition::name() const { return d->name; }
void FunctionDefinition::setName(const QString &name) { d->name = name; }

QString FunctionDefinition::description() const { return d->description; }
void FunctionDefinition::setDescription(const QString &description) { d->description = description; }

QJsonObject FunctionDefinition::parameters() const { return d->parameters; }
void FunctionDefinition::setParameters(const QJsonObject &parameters) { d->parameters = parameters; }

bool FunctionDefinition::isEmpty() const { return d->name.isEmpty(); }

QJsonObject FunctionDefinition::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("name"), d->name);
    detail::insertIfNotEmpty(json, QStringLiteral("description"), d->description);
    if (!d->parameters.isEmpty())
        json.insert(QStringLiteral("parameters"), d->parameters);
    return json;
}

FunctionDefinition FunctionDefinition::fromJson(const QJsonObject &json)
{
    FunctionDefinition function;
    function.d->name = detail::stringOr(json, QStringLiteral("name"));
    function.d->description = detail::stringOr(json, QStringLiteral("description"));
    function.d->parameters = json.value(QStringLiteral("parameters")).toObject();
    return function;
}

bool FunctionDefinition::operator==(const FunctionDefinition &other) const
{
    return d->name == other.d->name
        && d->description == other.d->description
        && d->parameters == other.d->parameters;
}

// ---------------------------------------------------------------------------
// Tool
// ---------------------------------------------------------------------------

class ToolData : public QSharedData
{
public:
    QString type = QStringLiteral("function");
    FunctionDefinition function;
};

Tool::Tool()
    : d(new ToolData)
{
}

Tool::Tool(FunctionDefinition function)
    : d(new ToolData)
{
    d->function = std::move(function);
}

Tool::Tool(const Tool &other) = default;
Tool::Tool(Tool &&other) noexcept = default;
Tool &Tool::operator=(const Tool &other) = default;
Tool &Tool::operator=(Tool &&other) noexcept = default;
Tool::~Tool() = default;

QString Tool::type() const { return d->type; }
void Tool::setType(const QString &type) { d->type = type; }

FunctionDefinition Tool::function() const { return d->function; }
void Tool::setFunction(const FunctionDefinition &function) { d->function = function; }

bool Tool::isEmpty() const { return d->function.isEmpty(); }

QJsonObject Tool::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("type"), d->type);
    json.insert(QStringLiteral("function"), d->function.toJson());
    return json;
}

Tool Tool::fromJson(const QJsonObject &json)
{
    Tool tool;
    tool.d->type = detail::stringOr(json, QStringLiteral("type"), QStringLiteral("function"));
    tool.d->function = FunctionDefinition::fromJson(json.value(QStringLiteral("function")).toObject());
    return tool;
}

Tool Tool::function(const QString &name, const QString &description, const QJsonObject &parameters)
{
    return Tool(FunctionDefinition(name, description, parameters));
}

bool Tool::operator==(const Tool &other) const
{
    return d->type == other.d->type && d->function == other.d->function;
}

} // namespace Core
} // namespace QtOpenAi
