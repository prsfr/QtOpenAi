// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QStringList>

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
// from `Q_CLASSINFO`, keyed by the path to what they describe. Write them with
// the macros below rather than by hand:
//
//     QTOPENAI_DOC("Someone to greet")                        // the object
//     QTOPENAI_DOC_PROPERTY(age, "Whole years")               // a property
//     QTOPENAI_DOC_METHOD(forecast, "Tomorrow's weather")     // a method
//     QTOPENAI_DOC_ARGUMENT(forecast, location, "City name")  // its argument
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

// The `doc` annotations on this class that describe nothing: a key naming a
// property, method or argument the class does not have. Such a key is always a
// mistake -- a typo, or a rename that left its annotation behind -- and it is
// silent, because a description that matches nothing simply never appears in
// the schema. Assert on this in a test and a renamed argument stops being able
// to quietly lose its documentation.
//
// Only annotations MetaSchema itself would read are considered, so a `doc:` key
// for an inherited property (which fromMetaObject does not emit) is reported.
QTOPENAI_CORE_EXPORT QStringList danglingAnnotations(const QMetaObject *meta);

template <typename T>
QStringList danglingAnnotations()
{
    return danglingAnnotations(&T::staticMetaObject);
}

} // namespace MetaSchema

} // namespace Core
} // namespace QtOpenAi

// --- Describing a class to the model ---------------------------------------
//
// Q_CLASSINFO takes a key and a value, and the key is a path this library
// assembles conventions into: a `doc` prefix, then the member, then the
// argument. Written out by hand that is three chances to be wrong -- a missing
// prefix, a stray colon, a misspelt name -- and every one of them fails
// silently, because an annotation that matches nothing is simply never read.
//
// These macros build the key from identifiers instead. They expand to exactly
// the Q_CLASSINFO you would have written, and moc concatenates the literals
// before it ever sees a key, so nothing here costs anything at runtime. The
// same trick Qt uses for QML_NAMED_ELEMENT.
//
// A typo is still a typo -- `QTOPENAI_DOC_PROPERTY(agee, ...)` compiles -- but
// it is now a *lone* mistake rather than one hidden among the punctuation, and
// MetaSchema::danglingAnnotations() finds it.

// The class itself: what this object or tool is.
#define QTOPENAI_DOC(description) Q_CLASSINFO("doc", description)

// One Q_PROPERTY, by name.
#define QTOPENAI_DOC_PROPERTY(property, description) Q_CLASSINFO("doc:" #property, description)

// One Q_INVOKABLE method, by name. Expands the same as QTOPENAI_DOC_PROPERTY
// because properties and methods share the key space -- a class with a property
// and a method of the same name gives them one description between them, which
// is worth knowing and has never yet been worth separating.
#define QTOPENAI_DOC_METHOD(method, description) Q_CLASSINFO("doc:" #method, description)

// One argument of one method. `argument` is the parameter name as the signature
// spells it -- the name moc recorded, which is the same name the generated
// schema advertises.
#define QTOPENAI_DOC_ARGUMENT(method, argument, description)                                       \
    Q_CLASSINFO("doc:" #method ":" #argument, description)
