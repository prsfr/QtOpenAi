// SPDX-License-Identifier: MIT
#pragma once

// Internal (private) helpers shared across the Core module's serialisation
// code. Not installed and not part of the public API.

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {
namespace detail {

// Insert a string field only when it is non-empty (OpenAI omits empty fields).
inline void insertIfNotEmpty(QJsonObject &object, const QString &key, const QString &value)
{
    if (!value.isEmpty())
        object.insert(key, value);
}

// Read an optional string; returns an empty QString when absent.
inline QString stringOr(const QJsonObject &object, const QString &key, const QString &fallback = {})
{
    const QJsonValue value = object.value(key);
    return value.isString() ? value.toString() : fallback;
}

} // namespace detail
} // namespace Core
} // namespace QtOpenAi
