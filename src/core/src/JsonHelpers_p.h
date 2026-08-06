// SPDX-License-Identifier: MIT
#pragma once

// Internal (private) helpers shared across the Core module's serialisation
// code. Not installed and not part of the public API.

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <optional>

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

// Insert a boolean flag only when it is set. For the fields that mean something
// by being there at all -- an acknowledgement's `deleted` -- rather than the ones
// whose `false` is a real answer.
inline void insertIfTrue(QJsonObject &object, const QString &key, bool value)
{
    if (value)
        object.insert(key, true);
}

// Insert a list of plain strings as a JSON array, only when it is non-empty.
inline void insertIfNotEmpty(QJsonObject &object, const QString &key, const QStringList &values)
{
    if (!values.isEmpty())
        object.insert(key, QJsonArray::fromStringList(values));
}

// Read an array of plain strings; returns an empty list when absent. Non-string
// entries decode to empty strings, the same way stringOr() treats a wrong type.
inline QStringList stringListOr(const QJsonObject &object, const QString &key)
{
    QStringList values;
    const QJsonArray array = object.value(key).toArray();
    values.reserve(array.size());
    for (const QJsonValue &value : array)
        values.append(value.toString());
    return values;
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

// Insert a value that is only present when the caller set it (the std::optional
// request/response parameters). An unset value leaves the field out entirely,
// which is what OpenAI means by "not specified".
template <typename T>
inline void insertIfSet(QJsonObject &object, const QString &key, const std::optional<T> &value)
{
    if (value)
        object.insert(key, QJsonValue(*value));
}

// Read a number that the API may report as null (or omit): a non-number decodes
// to an unset optional rather than an invented 0.
inline std::optional<double> optionalDouble(const QJsonObject &object, const QString &key)
{
    const QJsonValue value = object.value(key);
    return value.isDouble() ? std::optional<double>(value.toDouble()) : std::nullopt;
}

inline std::optional<int> optionalInt(const QJsonObject &object, const QString &key)
{
    const QJsonValue value = object.value(key);
    return value.isDouble() ? std::optional<int>(value.toInt()) : std::nullopt;
}

inline std::optional<bool> optionalBool(const QJsonObject &object, const QString &key)
{
    const QJsonValue value = object.value(key);
    return value.isBool() ? std::optional<bool>(value.toBool()) : std::nullopt;
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
