// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaType>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

// JSON-Schema derived from Qt's meta-object system.
//
// Everywhere this library takes a schema — a tool's `parameters`, a
// Structured Outputs `json_schema` — it has so far been hand-written, which
// means it can drift from the code it describes: rename a handler's argument
// and the schema still advertises the old name, with nothing to catch it. The
// meta-object system already knows the names and types, so the schema can be
// derived from them instead.
//
//     Q_INVOKABLE QString forecast(const QString &location, int days);
//     MetaSchema::fromMethod(receiver->metaObject(), "forecast");
//     // {"type":"object",
//     //  "properties":{"location":{"type":"string"},"days":{"type":"integer"}},
//     //  "required":["location","days"],"additionalProperties":false}
//
// The type mapping covers what a tool argument realistically is: strings,
// integers (kept distinct from floating-point numbers, as JSON-Schema does),
// booleans, string lists and other sequences, `QDateTime` as a date-time
// string, `Q_ENUM` as the closed set of its keys, and any `Q_GADGET` or
// `QObject` as a nested object built from its `Q_PROPERTY`s. A type outside
// that set yields an *empty* schema, which accepts anything — an honest "no
// constraint" rather than a guessed one.
//
// Every property and argument is listed in `required`, and objects are closed
// with `additionalProperties: false`. Both are what Structured Outputs demands
// in strict mode, and neither loses anything for a tool: a C++ signature has no
// absent parameters.
//
// Descriptions — the one thing the meta-object system does not carry — come
// from `Q_CLASSINFO` under a `doc` prefix, so they live next to the member they
// describe:
//
//     Q_CLASSINFO("doc", "Someone to greet")            // the object itself
//     Q_CLASSINFO("doc:age", "Whole years")             // a property
//     Q_CLASSINFO("doc:forecast:location", "City name") // a method argument
namespace MetaSchema {

// The schema for a Q_GADGET/QObject, built from its Q_PROPERTYs.
QTOPENAI_CORE_EXPORT QJsonObject fromMetaObject(const QMetaObject *meta);

// The same, for a type known at compile time.
template <typename T>
QJsonObject fromType()
{
    return fromMetaObject(&T::staticMetaObject);
}

// The schema for one invokable method's arguments, addressed by name. Returns
// an empty object when the method does not exist, so a caller that mistypes a
// name gets nothing rather than a wrong description.
QTOPENAI_CORE_EXPORT QJsonObject fromMethod(const QMetaObject *meta, const QString &method);

// The schema fragment for a single type; empty when the type is not one this
// mapping describes.
QTOPENAI_CORE_EXPORT QJsonObject fromMetaType(QMetaType type);

} // namespace MetaSchema

} // namespace Core
} // namespace QtOpenAi
