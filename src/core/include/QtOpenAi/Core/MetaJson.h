// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaType>
#include <QtCore/QVariant>

namespace QtOpenAi {
namespace Core {

// JSON carried in and out of a Q_GADGET or QObject through its Q_PROPERTYs.
//
// `MetaSchema` tells the model what shape to answer in; this is what turns that
// answer back into the type the schema was derived from, so a Structured
// Outputs round trip has one source of truth at both ends:
//
//     request.setResponseFormat(ResponseFormat::forType<Recipe>());
//     const Recipe recipe = MetaJson::parse<Recipe>(message.content());
//
// A value that does not fit its property is not written, and the read reports
// false -- properties that did fit are still written, so one bad field does not
// cost the whole object. Conversions follow what `MetaSchema` advertised: an
// enum is named by its key, a nested object is built through its own
// properties, and everything else goes through QVariant.
namespace MetaJson {

// One JSON value as the C++ type requested; an invalid QVariant when it does
// not fit.
QTOPENAI_CORE_EXPORT QVariant convert(const QJsonValue &value, QMetaType type);

// Write the properties named in the JSON. Returns false if any of them could
// not be written.
QTOPENAI_CORE_EXPORT bool readInto(QObject *object, const QJsonObject &json);
QTOPENAI_CORE_EXPORT bool readInto(const QMetaObject *meta, void *gadget, const QJsonObject &json);

// The properties as JSON -- the inverse of readInto.
QTOPENAI_CORE_EXPORT QJsonObject write(const QObject *object);
QTOPENAI_CORE_EXPORT QJsonObject write(const QMetaObject *meta, const void *gadget);

// A gadget built from JSON. Properties the JSON does not mention keep the value
// the default constructor gave them.
template <typename T>
T fromJson(const QJsonObject &json)
{
    T value;
    readInto(&T::staticMetaObject, &value, json);
    return value;
}

// The same, from a message's content -- what a model actually returns.
template <typename T>
T parse(const QString &content)
{
    return fromJson<T>(QJsonDocument::fromJson(content.toUtf8()).object());
}

template <typename T>
QJsonObject toJson(const T &gadget)
{
    return write(&T::staticMetaObject, &gadget);
}

} // namespace MetaJson

} // namespace Core
} // namespace QtOpenAi
