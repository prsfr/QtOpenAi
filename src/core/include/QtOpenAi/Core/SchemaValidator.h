// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QStringList>

namespace QtOpenAi {
namespace Core {

// A small JSON-Schema checker for the schemas this library actually exchanges.
//
// Tool arguments and Structured Outputs both arrive as model-generated JSON,
// which can be malformed in ways no C++ signature anticipates: a missing
// argument, a number where a string belongs, an enum value that does not exist.
// Validating against the schema that was advertised turns that into a precise
// message the model can act on, instead of a handler crashing on `toInt()`
// returning 0.
//
//     const QStringList errors = SchemaValidator::validate(schema, arguments);
//     // {"/days: expected integer, got string"}
//
// The supported vocabulary is deliberately the part that describes data rather
// than the part that composes schemas: `type` (including `integer` as a whole
// number), `enum`, `const`, `required`, `properties`, `additionalProperties`,
// `items`, `minimum`/`maximum` and their exclusive forms, `minLength`/
// `maxLength`/`pattern`, and `minItems`/`maxItems`. `$ref`, `allOf` and friends
// are not implemented; a schema using them is simply not constrained by the
// keywords this does not know, which keeps a partial understanding from
// rejecting valid data.
//
// This is the same vocabulary `MetaSchema` produces, so a schema derived from a
// meta-object is validated in full.
namespace SchemaValidator {

// Every constraint the value breaks, in document order; empty when it is valid.
// Each message is prefixed with a JSON-Pointer path to the offending value, so
// nested failures stay addressable.
QTOPENAI_CORE_EXPORT QStringList validate(const QJsonObject &schema, const QJsonValue &value);

// Whether the value satisfies the schema -- validate() without the reasons.
QTOPENAI_CORE_EXPORT bool isValid(const QJsonObject &schema, const QJsonValue &value);

} // namespace SchemaValidator

} // namespace Core
} // namespace QtOpenAi
