// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class CreateEvalRequestData;

// The body of a POST /evals request.
//
// The data-source config and the grader list are the two open unions of the
// Evals API, so they are passed through as raw JSON — the same choice Eval
// itself makes for the values it reads back.
class QTOPENAI_CORE_EXPORT CreateEvalRequest
{
public:
    CreateEvalRequest();
    CreateEvalRequest(QJsonObject dataSourceConfig, QJsonArray testingCriteria);
    CreateEvalRequest(const CreateEvalRequest &other);
    CreateEvalRequest(CreateEvalRequest &&other) noexcept;
    CreateEvalRequest &operator=(const CreateEvalRequest &other);
    CreateEvalRequest &operator=(CreateEvalRequest &&other) noexcept;
    ~CreateEvalRequest();

    void swap(CreateEvalRequest &other) noexcept { d.swap(other.d); }

    QString name() const;
    void setName(const QString &name);

    QJsonObject dataSourceConfig() const;
    void setDataSourceConfig(const QJsonObject &dataSourceConfig);

    QJsonArray testingCriteria() const;
    void setTestingCriteria(const QJsonArray &testingCriteria);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    QJsonObject toJson() const;

    bool operator==(const CreateEvalRequest &other) const;
    bool operator!=(const CreateEvalRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CreateEvalRequestData> d;
};

class CreateEvalRunRequestData;

// The body of a POST /evals/{eval_id}/runs request: what to run the eval
// against. `data_source` is an open union and is passed through verbatim.
class QTOPENAI_CORE_EXPORT CreateEvalRunRequest
{
public:
    CreateEvalRunRequest();
    explicit CreateEvalRunRequest(QJsonObject dataSource);
    CreateEvalRunRequest(const CreateEvalRunRequest &other);
    CreateEvalRunRequest(CreateEvalRunRequest &&other) noexcept;
    CreateEvalRunRequest &operator=(const CreateEvalRunRequest &other);
    CreateEvalRunRequest &operator=(CreateEvalRunRequest &&other) noexcept;
    ~CreateEvalRunRequest();

    void swap(CreateEvalRunRequest &other) noexcept { d.swap(other.d); }

    QString name() const;
    void setName(const QString &name);

    QJsonObject dataSource() const;
    void setDataSource(const QJsonObject &dataSource);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    QJsonObject toJson() const;

    bool operator==(const CreateEvalRunRequest &other) const;
    bool operator!=(const CreateEvalRunRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CreateEvalRunRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::CreateEvalRequest)
Q_DECLARE_SHARED(QtOpenAi::Core::CreateEvalRunRequest)
