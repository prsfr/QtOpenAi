// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

// The ordered name/value pairs a multipart/form-data request body is made of.
//
// Ordered because the API reads repeated fields -- `include[]`,
// `timestamp_granularities[]` -- in the order they arrive, so a QList of pairs
// rather than a map.
//
// Named here, once, rather than in each request type. Ten public classes used to
// declare an identical nested `FormField`, which gave one concept ten type names:
// nothing could be written generically over "a request that has form fields",
// and the ordering guarantee above appeared in the documentation ten times as
// ten unrelated nested types. The endpoints whose body is a form are
// transcription, translation, image edits and variations, file uploads, videos,
// skills and voices.
using FormField = QPair<QString, QString>;
using FormFields = QList<FormField>;

} // namespace Core
} // namespace QtOpenAi
