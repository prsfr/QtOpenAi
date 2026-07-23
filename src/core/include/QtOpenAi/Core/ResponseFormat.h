// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class ResponseFormatData;

// Constrained-decoding format for chat/responses requests. Models the union:
//   { "type": "text" }
//   { "type": "json_object" }
//   { "type": "json_schema", json_schema: { name, description, schema, strict } }
//
// The Chat Completions `response_format` nests the schema fields under a
// `json_schema` object (toJson); the Responses API `text.format` inlines them
// (toTextFormatJson).
class QTOPENAI_CORE_EXPORT ResponseFormat
{
public:
    ResponseFormat();
    explicit ResponseFormat(const QString &type);
    ResponseFormat(const ResponseFormat &other);
    ResponseFormat(ResponseFormat &&other) noexcept;
    ResponseFormat &operator=(const ResponseFormat &other);
    ResponseFormat &operator=(ResponseFormat &&other) noexcept;
    ~ResponseFormat();

    void swap(ResponseFormat &other) noexcept { d.swap(other.d); }

    // "text", "json_object", or "json_schema".
    QString type() const;
    void setType(const QString &type);

    // json_schema fields (ignored for other types).
    QString name() const;
    void setName(const QString &name);

    QString description() const;
    void setDescription(const QString &description);

    QJsonObject schema() const;
    void setSchema(const QJsonObject &schema);

    bool strict() const;
    void setStrict(bool strict);

    bool isNull() const; // no type set

    // Convenience builders.
    static ResponseFormat text();
    static ResponseFormat jsonObject();
    static ResponseFormat jsonSchema(const QString &name, const QJsonObject &schema,
                                     bool strict = true, const QString &description = {});

    // Chat Completions `response_format` shape (schema nested under json_schema).
    QJsonObject toJson() const;
    static ResponseFormat fromJson(const QJsonObject &json);

    // Responses API `text.format` shape (schema fields inlined).
    QJsonObject toTextFormatJson() const;

    bool operator==(const ResponseFormat &other) const;
    bool operator!=(const ResponseFormat &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ResponseFormatData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ResponseFormat)
