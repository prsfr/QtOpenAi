// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/CreateEvalRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

// --- CreateEvalRequest -----------------------------------------------------

class CreateEvalRequestData : public QSharedData
{
public:
    QString name;
    QJsonObject dataSourceConfig;
    QJsonArray testingCriteria;
    QJsonObject metadata;
};

CreateEvalRequest::CreateEvalRequest()
    : d(new CreateEvalRequestData)
{ }

CreateEvalRequest::CreateEvalRequest(QJsonObject dataSourceConfig, QJsonArray testingCriteria)
    : d(new CreateEvalRequestData)
{
    d->dataSourceConfig = std::move(dataSourceConfig);
    d->testingCriteria = std::move(testingCriteria);
}

CreateEvalRequest::CreateEvalRequest(const CreateEvalRequest &other) = default;
CreateEvalRequest::CreateEvalRequest(CreateEvalRequest &&other) noexcept = default;
CreateEvalRequest &CreateEvalRequest::operator=(const CreateEvalRequest &other) = default;
CreateEvalRequest &CreateEvalRequest::operator=(CreateEvalRequest &&other) noexcept = default;
CreateEvalRequest::~CreateEvalRequest() = default;

QString CreateEvalRequest::name() const { return d->name; }
void CreateEvalRequest::setName(const QString &name) { d->name = name; }

QJsonObject CreateEvalRequest::dataSourceConfig() const { return d->dataSourceConfig; }
void CreateEvalRequest::setDataSourceConfig(const QJsonObject &dataSourceConfig)
{
    d->dataSourceConfig = dataSourceConfig;
}

QJsonArray CreateEvalRequest::testingCriteria() const { return d->testingCriteria; }
void CreateEvalRequest::setTestingCriteria(const QJsonArray &testingCriteria)
{
    d->testingCriteria = testingCriteria;
}

QJsonObject CreateEvalRequest::metadata() const { return d->metadata; }
void CreateEvalRequest::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

QJsonObject CreateEvalRequest::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    if (!d->dataSourceConfig.isEmpty())
        json.insert(QStringLiteral("data_source_config"), d->dataSourceConfig);
    if (!d->testingCriteria.isEmpty())
        json.insert(QStringLiteral("testing_criteria"), d->testingCriteria);
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    return json;
}

bool CreateEvalRequest::operator==(const CreateEvalRequest &other) const
{
    return d->name == other.d->name && d->dataSourceConfig == other.d->dataSourceConfig
           && d->testingCriteria == other.d->testingCriteria && d->metadata == other.d->metadata;
}

// --- CreateEvalRunRequest --------------------------------------------------

class CreateEvalRunRequestData : public QSharedData
{
public:
    QString name;
    QJsonObject dataSource;
    QJsonObject metadata;
};

CreateEvalRunRequest::CreateEvalRunRequest()
    : d(new CreateEvalRunRequestData)
{ }

CreateEvalRunRequest::CreateEvalRunRequest(QJsonObject dataSource)
    : d(new CreateEvalRunRequestData)
{
    d->dataSource = std::move(dataSource);
}

CreateEvalRunRequest::CreateEvalRunRequest(const CreateEvalRunRequest &other) = default;
CreateEvalRunRequest::CreateEvalRunRequest(CreateEvalRunRequest &&other) noexcept = default;
CreateEvalRunRequest &CreateEvalRunRequest::operator=(const CreateEvalRunRequest &other) = default;
CreateEvalRunRequest &CreateEvalRunRequest::operator=(CreateEvalRunRequest &&other) noexcept
        = default;
CreateEvalRunRequest::~CreateEvalRunRequest() = default;

QString CreateEvalRunRequest::name() const { return d->name; }
void CreateEvalRunRequest::setName(const QString &name) { d->name = name; }

QJsonObject CreateEvalRunRequest::dataSource() const { return d->dataSource; }
void CreateEvalRunRequest::setDataSource(const QJsonObject &dataSource)
{
    d->dataSource = dataSource;
}

QJsonObject CreateEvalRunRequest::metadata() const { return d->metadata; }
void CreateEvalRunRequest::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

QJsonObject CreateEvalRunRequest::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    if (!d->dataSource.isEmpty())
        json.insert(QStringLiteral("data_source"), d->dataSource);
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    return json;
}

bool CreateEvalRunRequest::operator==(const CreateEvalRunRequest &other) const
{
    return d->name == other.d->name && d->dataSource == other.d->dataSource
           && d->metadata == other.d->metadata;
}

} // namespace Core
} // namespace QtOpenAi
