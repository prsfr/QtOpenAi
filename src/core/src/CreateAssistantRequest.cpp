// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/CreateAssistantRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class CreateAssistantRequestData : public QSharedData
{
public:
    QString model;
    QString name;
    QString description;
    QString instructions;
    QJsonArray tools;
    QJsonObject toolResources;
    QJsonObject metadata;
    std::optional<double> temperature;
    std::optional<double> topP;
    QJsonValue responseFormat = QJsonValue::Undefined;
};

CreateAssistantRequest::CreateAssistantRequest()
    : d(new CreateAssistantRequestData)
{ }

CreateAssistantRequest::CreateAssistantRequest(QString model)
    : d(new CreateAssistantRequestData)
{
    d->model = std::move(model);
}

CreateAssistantRequest::CreateAssistantRequest(const CreateAssistantRequest &other) = default;
CreateAssistantRequest::CreateAssistantRequest(CreateAssistantRequest &&other) noexcept = default;
CreateAssistantRequest &CreateAssistantRequest::operator=(const CreateAssistantRequest &other)
        = default;
CreateAssistantRequest &CreateAssistantRequest::operator=(CreateAssistantRequest &&other) noexcept
        = default;
CreateAssistantRequest::~CreateAssistantRequest() = default;

QString CreateAssistantRequest::model() const { return d->model; }
void CreateAssistantRequest::setModel(const QString &model) { d->model = model; }

QString CreateAssistantRequest::name() const { return d->name; }
void CreateAssistantRequest::setName(const QString &name) { d->name = name; }

QString CreateAssistantRequest::description() const { return d->description; }
void CreateAssistantRequest::setDescription(const QString &description)
{
    d->description = description;
}

QString CreateAssistantRequest::instructions() const { return d->instructions; }
void CreateAssistantRequest::setInstructions(const QString &instructions)
{
    d->instructions = instructions;
}

QJsonArray CreateAssistantRequest::tools() const { return d->tools; }
void CreateAssistantRequest::setTools(const QJsonArray &tools) { d->tools = tools; }

void CreateAssistantRequest::addTool(const Tool &tool) { d->tools.append(tool.toJson()); }
void CreateAssistantRequest::addTool(const QJsonObject &tool) { d->tools.append(tool); }

void CreateAssistantRequest::addCodeInterpreterTool()
{
    d->tools.append(QJsonObject {{QStringLiteral("type"), QStringLiteral("code_interpreter")}});
}

void CreateAssistantRequest::addFileSearchTool()
{
    d->tools.append(QJsonObject {{QStringLiteral("type"), QStringLiteral("file_search")}});
}

QJsonObject CreateAssistantRequest::toolResources() const { return d->toolResources; }
void CreateAssistantRequest::setToolResources(const QJsonObject &toolResources)
{
    d->toolResources = toolResources;
}

QJsonObject CreateAssistantRequest::metadata() const { return d->metadata; }
void CreateAssistantRequest::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

std::optional<double> CreateAssistantRequest::temperature() const { return d->temperature; }
void CreateAssistantRequest::setTemperature(double temperature) { d->temperature = temperature; }

std::optional<double> CreateAssistantRequest::topP() const { return d->topP; }
void CreateAssistantRequest::setTopP(double topP) { d->topP = topP; }

QJsonValue CreateAssistantRequest::responseFormat() const { return d->responseFormat; }
void CreateAssistantRequest::setResponseFormat(const QJsonValue &responseFormat)
{
    d->responseFormat = responseFormat;
}

void CreateAssistantRequest::setResponseFormat(const ResponseFormat &responseFormat)
{
    d->responseFormat = responseFormat.toJson();
}

QJsonObject CreateAssistantRequest::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("model"), d->model);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNotEmpty(json, QStringLiteral("description"), d->description);
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

bool CreateAssistantRequest::operator==(const CreateAssistantRequest &other) const
{
    return d->model == other.d->model && d->name == other.d->name
           && d->description == other.d->description && d->instructions == other.d->instructions
           && d->tools == other.d->tools && d->toolResources == other.d->toolResources
           && d->metadata == other.d->metadata && d->temperature == other.d->temperature
           && d->topP == other.d->topP && d->responseFormat == other.d->responseFormat;
}

} // namespace Core
} // namespace QtOpenAi
