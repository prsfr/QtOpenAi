// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Model.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ModelData : public QSharedData
{
public:
    QString id;
    QString object = QStringLiteral("model");
    qint64 created = 0;
    QString ownedBy;
};

Model::Model()
    : d(new ModelData)
{ }

Model::Model(const Model &other) = default;
Model::Model(Model &&other) noexcept = default;
Model &Model::operator=(const Model &other) = default;
Model &Model::operator=(Model &&other) noexcept = default;
Model::~Model() = default;

QString Model::id() const { return d->id; }
void Model::setId(const QString &id) { d->id = id; }

QString Model::object() const { return d->object; }
void Model::setObject(const QString &object) { d->object = object; }

qint64 Model::created() const { return d->created; }
void Model::setCreated(qint64 created) { d->created = created; }

QString Model::ownedBy() const { return d->ownedBy; }
void Model::setOwnedBy(const QString &ownedBy) { d->ownedBy = ownedBy; }

QJsonObject Model::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("id"), d->id);
    json.insert(QStringLiteral("object"), d->object);
    json.insert(QStringLiteral("created"), d->created);
    detail::insertIfNotEmpty(json, QStringLiteral("owned_by"), d->ownedBy);
    return json;
}

Model Model::fromJson(const QJsonObject &json)
{
    Model model;
    model.d->id = detail::stringOr(json, QStringLiteral("id"));
    model.d->object = detail::stringOr(json, QStringLiteral("object"), QStringLiteral("model"));
    model.d->created = static_cast<qint64>(json.value(QStringLiteral("created")).toDouble());
    model.d->ownedBy = detail::stringOr(json, QStringLiteral("owned_by"));
    return model;
}

bool Model::operator==(const Model &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->created == other.d->created
           && d->ownedBy == other.d->ownedBy;
}

} // namespace Core
} // namespace QtOpenAi
