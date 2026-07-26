// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Eval.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class EvalData : public QSharedData
{
public:
    QString id;
    QString object;
    QString name;
    qint64 createdAt = 0;
    QJsonObject dataSourceConfig;
    QJsonArray testingCriteria;
    QJsonObject metadata;
};

Eval::Eval()
    : d(new EvalData)
{ }

Eval::Eval(const Eval &other) = default;
Eval::Eval(Eval &&other) noexcept = default;
Eval &Eval::operator=(const Eval &other) = default;
Eval &Eval::operator=(Eval &&other) noexcept = default;
Eval::~Eval() = default;

QString Eval::id() const { return d->id; }
void Eval::setId(const QString &id) { d->id = id; }

QString Eval::object() const { return d->object; }
void Eval::setObject(const QString &object) { d->object = object; }

QString Eval::name() const { return d->name; }
void Eval::setName(const QString &name) { d->name = name; }

qint64 Eval::createdAt() const { return d->createdAt; }
void Eval::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QJsonObject Eval::dataSourceConfig() const { return d->dataSourceConfig; }
void Eval::setDataSourceConfig(const QJsonObject &dataSourceConfig)
{
    d->dataSourceConfig = dataSourceConfig;
}

QJsonArray Eval::testingCriteria() const { return d->testingCriteria; }
void Eval::setTestingCriteria(const QJsonArray &testingCriteria)
{
    d->testingCriteria = testingCriteria;
}

QJsonObject Eval::metadata() const { return d->metadata; }
void Eval::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

QJsonObject Eval::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    if (!d->dataSourceConfig.isEmpty())
        json.insert(QStringLiteral("data_source_config"), d->dataSourceConfig);
    if (!d->testingCriteria.isEmpty())
        json.insert(QStringLiteral("testing_criteria"), d->testingCriteria);
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    return json;
}

Eval Eval::fromJson(const QJsonObject &json)
{
    Eval eval;
    // The deletion acknowledgement names the id `eval_id`; accept both.
    eval.d->id = detail::stringOr(json, QStringLiteral("id"),
                                  detail::stringOr(json, QStringLiteral("eval_id")));
    eval.d->object = detail::stringOr(json, QStringLiteral("object"));
    eval.d->name = detail::stringOr(json, QStringLiteral("name"));
    eval.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    eval.d->dataSourceConfig = json.value(QStringLiteral("data_source_config")).toObject();
    eval.d->testingCriteria = json.value(QStringLiteral("testing_criteria")).toArray();
    eval.d->metadata = json.value(QStringLiteral("metadata")).toObject();
    return eval;
}

bool Eval::operator==(const Eval &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->name == other.d->name
           && d->createdAt == other.d->createdAt && d->dataSourceConfig == other.d->dataSourceConfig
           && d->testingCriteria == other.d->testingCriteria && d->metadata == other.d->metadata;
}

} // namespace Core
} // namespace QtOpenAi
