// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/SchemaValidator.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QRegularExpression>

#include <cmath>

namespace QtOpenAi {
namespace Core {
namespace SchemaValidator {

namespace {

constexpr QLatin1String kType("type");
constexpr QLatin1String kEnum("enum");
constexpr QLatin1String kConst("const");
constexpr QLatin1String kRequired("required");
constexpr QLatin1String kProperties("properties");
constexpr QLatin1String kAdditionalProperties("additionalProperties");
constexpr QLatin1String kItems("items");

// The JSON type name for a value, in the vocabulary `type` uses.
QString typeOf(const QJsonValue &value)
{
    switch (value.type()) {
    case QJsonValue::Null:
        return QStringLiteral("null");
    case QJsonValue::Bool:
        return QStringLiteral("boolean");
    case QJsonValue::Double:
        return QStringLiteral("number");
    case QJsonValue::String:
        return QStringLiteral("string");
    case QJsonValue::Array:
        return QStringLiteral("array");
    case QJsonValue::Object:
        return QStringLiteral("object");
    case QJsonValue::Undefined:
        break;
    }
    return QStringLiteral("undefined");
}

bool matchesType(const QJsonValue &value, const QString &type)
{
    // JSON has one numeric type; `integer` is a number that happens to be whole,
    // which is how JSON-Schema defines it too.
    if (type == QLatin1String("integer")) {
        double whole = 0;
        return value.isDouble() && std::modf(value.toDouble(), &whole) == 0.0;
    }
    if (type == QLatin1String("number"))
        return value.isDouble();
    return typeOf(value) == type;
}

// A value rendered for an error message: JSON as the model wrote it, so the
// message quotes strings and leaves numbers bare.
QString display(const QJsonValue &value)
{
    const QJsonArray wrapper {value};
    const QString text = QString::fromUtf8(QJsonDocument(wrapper).toJson(QJsonDocument::Compact));
    return text.mid(1, text.size() - 2);
}

QString join(const QStringList &parts) { return parts.join(QStringLiteral(" or ")); }

class Validator
{
public:
    QStringList errors;

    void check(const QJsonObject &schema, const QJsonValue &value, const QString &path);

private:
    void fail(const QString &path, const QString &message)
    {
        errors.append(path.isEmpty() ? message : path + QStringLiteral(": ") + message);
    }

    static QString child(const QString &path, const QString &key)
    {
        return path + QLatin1Char('/') + key;
    }

    void checkType(const QJsonObject &schema, const QJsonValue &value, const QString &path);
    void checkValues(const QJsonObject &schema, const QJsonValue &value, const QString &path);
    void checkNumber(const QJsonObject &schema, const QJsonValue &value, const QString &path);
    void checkString(const QJsonObject &schema, const QJsonValue &value, const QString &path);
    void checkArray(const QJsonObject &schema, const QJsonValue &value, const QString &path);
    void checkObject(const QJsonObject &schema, const QJsonValue &value, const QString &path);
};

void Validator::checkType(const QJsonObject &schema, const QJsonValue &value, const QString &path)
{
    const QJsonValue type = schema.value(kType);
    if (type.isUndefined())
        return;

    // `type` is either one name or a choice between several.
    QStringList allowed;
    if (type.isArray()) {
        const QJsonArray names = type.toArray();
        for (const QJsonValue &name : names)
            allowed.append(name.toString());
    } else {
        allowed.append(type.toString());
    }

    for (const QString &name : std::as_const(allowed)) {
        if (matchesType(value, name))
            return;
    }
    fail(path, QStringLiteral("expected %1, got %2").arg(join(allowed), typeOf(value)));
}

void Validator::checkValues(const QJsonObject &schema, const QJsonValue &value, const QString &path)
{
    const QJsonValue constant = schema.value(kConst);
    if (!constant.isUndefined() && value != constant) {
        fail(path, QStringLiteral("expected %1, got %2").arg(display(constant), display(value)));
        return;
    }

    const QJsonValue allowed = schema.value(kEnum);
    if (!allowed.isArray())
        return;

    const QJsonArray values = allowed.toArray();
    QStringList rendered;
    for (const QJsonValue &candidate : values) {
        if (candidate == value)
            return;
        rendered.append(display(candidate));
    }
    if (!rendered.isEmpty()) {
        fail(path, QStringLiteral("expected one of %1, got %2")
                           .arg(rendered.join(QStringLiteral(", ")), display(value)));
    }
}

void Validator::checkNumber(const QJsonObject &schema, const QJsonValue &value, const QString &path)
{
    if (!value.isDouble())
        return;

    const double number = value.toDouble();
    const auto compare = [&](QLatin1String keyword, const QString &relation, bool exclusive,
                             bool below) {
        const QJsonValue bound = schema.value(keyword);
        if (!bound.isDouble())
            return;
        const double limit = bound.toDouble();
        const bool broken = below ? (exclusive ? number <= limit : number < limit)
                                  : (exclusive ? number >= limit : number > limit);
        if (broken) {
            fail(path, QStringLiteral("%1 is not %2 %3")
                               .arg(display(value), relation, display(QJsonValue(limit))));
        }
    };

    compare(QLatin1String("minimum"), QStringLiteral("at least"), false, true);
    compare(QLatin1String("maximum"), QStringLiteral("at most"), false, false);
    compare(QLatin1String("exclusiveMinimum"), QStringLiteral("greater than"), true, true);
    compare(QLatin1String("exclusiveMaximum"), QStringLiteral("less than"), true, false);
}

void Validator::checkString(const QJsonObject &schema, const QJsonValue &value, const QString &path)
{
    if (!value.isString())
        return;

    const QString text = value.toString();
    const QJsonValue minimum = schema.value(QLatin1String("minLength"));
    if (minimum.isDouble() && text.size() < minimum.toInt()) {
        fail(path,
             QStringLiteral("shorter than %1 characters").arg(QString::number(minimum.toInt())));
    }
    const QJsonValue maximum = schema.value(QLatin1String("maxLength"));
    if (maximum.isDouble() && text.size() > maximum.toInt()) {
        fail(path,
             QStringLiteral("longer than %1 characters").arg(QString::number(maximum.toInt())));
    }

    const QString pattern = schema.value(QLatin1String("pattern")).toString();
    if (!pattern.isEmpty()) {
        const QRegularExpression expression(pattern);
        // An unparseable pattern constrains nothing -- rejecting the data for a
        // fault in the schema would blame the wrong side.
        if (expression.isValid() && !expression.match(text).hasMatch())
            fail(path, QStringLiteral("does not match %1").arg(pattern));
    }
}

void Validator::checkArray(const QJsonObject &schema, const QJsonValue &value, const QString &path)
{
    if (!value.isArray())
        return;

    const QJsonArray items = value.toArray();
    const QJsonValue minimum = schema.value(QLatin1String("minItems"));
    if (minimum.isDouble() && items.size() < minimum.toInt())
        fail(path, QStringLiteral("has fewer than %1 items").arg(QString::number(minimum.toInt())));
    const QJsonValue maximum = schema.value(QLatin1String("maxItems"));
    if (maximum.isDouble() && items.size() > maximum.toInt())
        fail(path, QStringLiteral("has more than %1 items").arg(QString::number(maximum.toInt())));

    const QJsonValue element = schema.value(kItems);
    if (!element.isObject())
        return;
    for (int i = 0; i < items.size(); ++i)
        check(element.toObject(), items.at(i), child(path, QString::number(i)));
}

void Validator::checkObject(const QJsonObject &schema, const QJsonValue &value, const QString &path)
{
    if (!value.isObject())
        return;

    const QJsonObject object = value.toObject();

    const QJsonArray required = schema.value(kRequired).toArray();
    for (const QJsonValue &name : required) {
        if (!object.contains(name.toString()))
            fail(path, QStringLiteral("missing required property '%1'").arg(name.toString()));
    }

    const QJsonObject properties = schema.value(kProperties).toObject();
    for (auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
        // An absent property is the business of `required`, not of its schema.
        if (it.value().isObject() && object.contains(it.key()))
            check(it.value().toObject(), object.value(it.key()), child(path, it.key()));
    }

    const QJsonValue additional = schema.value(kAdditionalProperties);
    if (additional.isUndefined())
        return;
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        if (properties.contains(it.key()))
            continue;
        if (additional.isObject())
            check(additional.toObject(), it.value(), child(path, it.key()));
        else if (!additional.toBool(true))
            fail(path, QStringLiteral("unexpected property '%1'").arg(it.key()));
    }
}

void Validator::check(const QJsonObject &schema, const QJsonValue &value, const QString &path)
{
    // An empty schema constrains nothing, which every keyword below agrees on
    // anyway -- this just says so without walking them.
    if (schema.isEmpty())
        return;

    const int before = errors.size();
    checkType(schema, value, path);
    // A value of the wrong type breaks every other keyword too; one clear
    // message beats a cascade of consequences.
    if (errors.size() != before)
        return;

    checkValues(schema, value, path);
    checkNumber(schema, value, path);
    checkString(schema, value, path);
    checkArray(schema, value, path);
    checkObject(schema, value, path);
}

} // namespace

QStringList validate(const QJsonObject &schema, const QJsonValue &value)
{
    Validator validator;
    validator.check(schema, value, QString());
    return validator.errors;
}

bool isValid(const QJsonObject &schema, const QJsonValue &value)
{
    return validate(schema, value).isEmpty();
}

} // namespace SchemaValidator
} // namespace Core
} // namespace QtOpenAi
