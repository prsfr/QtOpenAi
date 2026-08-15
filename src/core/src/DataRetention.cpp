// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/DataRetention.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class DataRetentionData : public QSharedData
{
public:
    QString object;
    QString type;
};

DataRetention::DataRetention()
    : d(new DataRetentionData)
{ }

DataRetention::DataRetention(const DataRetention &other) = default;
DataRetention::DataRetention(DataRetention &&other) noexcept = default;
DataRetention &DataRetention::operator=(const DataRetention &other) = default;
DataRetention &DataRetention::operator=(DataRetention &&other) noexcept = default;
DataRetention::~DataRetention() = default;

QString DataRetention::object() const { return d->object; }
void DataRetention::setObject(const QString &object) { d->object = object; }

QString DataRetention::type() const { return d->type; }
void DataRetention::setType(const QString &type) { d->type = type; }

QJsonObject DataRetention::toJson() const
{
    // The resource's own shape, which is what this type models. The *update*
    // body is a different shape with a different field name -- see the header --
    // and Organization::setDataRetention() builds it, so that nobody can send
    // this object as an update and have `type` silently ignored.
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("type"), d->type);
    return json;
}

DataRetention DataRetention::fromJson(const QJsonObject &json)
{
    DataRetention retention;
    retention.d->object = detail::stringOr(json, QStringLiteral("object"));
    retention.d->type = detail::stringOr(json, QStringLiteral("type"));
    return retention;
}

bool DataRetention::operator==(const DataRetention &other) const
{
    return d->object == other.d->object && d->type == other.d->type;
}

} // namespace Core
} // namespace QtOpenAi
