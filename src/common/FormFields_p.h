// SPDX-License-Identifier: MIT
#pragma once

// Internal (private) helpers for the multipart/form-data conventions -- the
// same job JsonHelpers_p.h does for the JSON body, for the handful of endpoints
// whose request is a form instead. Not installed and not part of the public API.
//
// A form has no null and no types: a field is either present as text or not
// there at all, so "unset" and "empty" are the same wire state and every
// optional field is written as a test before an append. Six request types wrote
// those tests out longhand -- seventeen times for a plain string alone -- which
// is one convention spelled seventeen times rather than named once.

#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <optional>

namespace QtOpenAi {
namespace Core {
namespace detail {

// The type every multipart request type spells as its own FormField: an ordered
// name/value pair, ordered because the API reads repeated fields in order.
using FormFields = QList<QPair<QString, QString>>;

// Append a text field only when it has something to say. An empty string would
// be sent as a present-but-blank field, which is not what an unset one means.
inline void appendIfNotEmpty(FormFields &fields, const QString &name, const QString &value)
{
    if (!value.isEmpty())
        fields.append({name, value});
}

// Append a number the caller may not have set. Stringified with QString::number
// rather than a locale-aware conversion: the server reads C, not the user's
// decimal separator.
template <typename T>
inline void appendIfSet(FormFields &fields, const QString &name, const std::optional<T> &value)
{
    if (value)
        fields.append({name, QString::number(*value)});
}

// A boolean travels as the word, not as 0/1 -- QString::number(bool) would send
// the digit, which the API does not accept.
inline void appendIfSet(FormFields &fields, const QString &name, const std::optional<bool> &value)
{
    if (value)
        fields.append({name, *value ? QStringLiteral("true") : QStringLiteral("false")});
}

// Append an array field, which a form expresses by repeating the field once per
// entry. `name` carries the `[]` suffix the API spells it with, because that is
// part of the field's name rather than something added here.
inline void appendEach(FormFields &fields, const QString &name, const QStringList &values)
{
    for (const QString &value : values)
        fields.append({name, value});
}

} // namespace detail
} // namespace Core
} // namespace QtOpenAi
