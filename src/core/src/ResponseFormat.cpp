// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ResponseFormat.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ResponseFormatData : public QSharedData
{
public:
    QString type;
    QString name;
    QString description;
    QJsonObject schema;
    bool strict = true;
};

ResponseFormat::ResponseFormat()
    : d(new ResponseFormatData)
{ }

ResponseFormat::ResponseFormat(const QString &type)
    : d(new ResponseFormatData)
{
    d->type = type;
}

ResponseFormat::ResponseFormat(const ResponseFormat &other) = default;
ResponseFormat::ResponseFormat(ResponseFormat &&other) noexcept = default;
ResponseFormat &ResponseFormat::operator=(const ResponseFormat &other) = default;
ResponseFormat &ResponseFormat::operator=(ResponseFormat &&other) noexcept = default;
ResponseFormat::~ResponseFormat() = default;

QString ResponseFormat::type() const { return d->type; }
void ResponseFormat::setType(const QString &type) { d->type = type; }

QString ResponseFormat::name() const { return d->name; }
void ResponseFormat::setName(const QString &name) { d->name = name; }

QString ResponseFormat::description() const { return d->description; }
void ResponseFormat::setDescription(const QString &description) { d->description = description; }

QJsonObject ResponseFormat::schema() const { return d->schema; }
void ResponseFormat::setSchema(const QJsonObject &schema) { d->schema = schema; }

bool ResponseFormat::strict() const { return d->strict; }
void ResponseFormat::setStrict(bool strict) { d->strict = strict; }

bool ResponseFormat::isNull() const { return d->type.isEmpty(); }

ResponseFormat ResponseFormat::text() { return ResponseFormat(QStringLiteral("text")); }
ResponseFormat ResponseFormat::jsonObject()
{
    return ResponseFormat(QStringLiteral("json_object"));
}

ResponseFormat ResponseFormat::jsonSchema(const QString &name, const QJsonObject &schema,
                                          bool strict, const QString &description)
{
    ResponseFormat format(QStringLiteral("json_schema"));
    format.d->name = name;
    format.d->schema = schema;
    format.d->strict = strict;
    format.d->description = description;
    return format;
}

QJsonObject ResponseFormat::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("type"), d->type);
    if (d->type == QLatin1String("json_schema")) {
        QJsonObject jsonSchema;
        jsonSchema.insert(QStringLiteral("name"), d->name);
        detail::insertIfNotEmpty(jsonSchema, QStringLiteral("description"), d->description);
        jsonSchema.insert(QStringLiteral("schema"), d->schema);
        jsonSchema.insert(QStringLiteral("strict"), d->strict);
        json.insert(QStringLiteral("json_schema"), jsonSchema);
    }
    return json;
}

QJsonObject ResponseFormat::toTextFormatJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("type"), d->type);
    if (d->type == QLatin1String("json_schema")) {
        json.insert(QStringLiteral("name"), d->name);
        detail::insertIfNotEmpty(json, QStringLiteral("description"), d->description);
        json.insert(QStringLiteral("schema"), d->schema);
        json.insert(QStringLiteral("strict"), d->strict);
    }
    return json;
}

ResponseFormat ResponseFormat::fromJson(const QJsonObject &json)
{
    ResponseFormat format;
    format.d->type = detail::stringOr(json, QStringLiteral("type"));
    // Accept both the nested (chat) and inlined (responses) shapes.
    const QJsonObject nested = json.value(QStringLiteral("json_schema")).toObject();
    const QJsonObject source = nested.isEmpty() ? json : nested;
    format.d->name = detail::stringOr(source, QStringLiteral("name"));
    format.d->description = detail::stringOr(source, QStringLiteral("description"));
    format.d->schema = source.value(QStringLiteral("schema")).toObject();
    if (source.contains(QStringLiteral("strict")))
        format.d->strict = source.value(QStringLiteral("strict")).toBool();
    return format;
}

bool ResponseFormat::operator==(const ResponseFormat &other) const
{
    return d->type == other.d->type && d->name == other.d->name
           && d->description == other.d->description && d->schema == other.d->schema
           && d->strict == other.d->strict;
}

} // namespace Core
} // namespace QtOpenAi
