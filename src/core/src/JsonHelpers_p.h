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

// Insert a 64-bit integer field only when it is non-zero (OpenAI omits absent
// timestamps and byte counts rather than sending a 0).
inline void insertIfNonZero(QJsonObject &object, const QString &key, qint64 value)
{
    if (value != 0)
        object.insert(key, value);
}

// Read an optional string; returns an empty QString when absent.
inline QString stringOr(const QJsonObject &object, const QString &key, const QString &fallback = {})
{
    const QJsonValue value = object.value(key);
    return value.isString() ? value.toString() : fallback;
}

// Read an optional 64-bit integer (timestamp, byte count). Goes through QVariant
// because QJsonValue::toDouble() loses precision beyond 2^53.
inline qint64 int64Or(const QJsonObject &object, const QString &key, qint64 fallback = 0)
{
    const QJsonValue value = object.value(key);
    return value.isDouble() ? value.toVariant().toLongLong() : fallback;
}

// Merge caller-supplied provider-specific fields into a request body, without
// overriding keys the typed serialiser already set (OpenAI `extra_body`).
inline void mergeExtraBody(QJsonObject &object, const QJsonObject &extra)
{
    for (auto it = extra.constBegin(); it != extra.constEnd(); ++it) {
        if (!object.contains(it.key()))
            object.insert(it.key(), it.value());
    }
}

} // namespace detail
} // namespace Core
} // namespace QtOpenAi
