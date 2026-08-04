// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/MetaJson.h"

#include <QtCore/QJsonArray>
#include <QtCore/QMetaEnum>
#include <QtCore/QMetaProperty>

namespace QtOpenAi {
namespace Core {
namespace MetaJson {

namespace {

// The enumerator behind a registered enum type, or an invalid QMetaEnum.
QMetaEnum enumeratorOf(QMetaType type)
{
    const QMetaObject *meta = type.metaObject();
    if (!meta)
        return {};
    const QByteArray name = QByteArray(type.name()).split(':').last();
    const int index = meta->indexOfEnumerator(name.constData());
    return index < 0 ? QMetaEnum() : meta->enumerator(index);
}

// Reading and writing a property differ only in whether there is a QObject to
// address; everything above this point is the same for both.
QVariant readProperty(const QMetaProperty &property, const QObject *object, const void *gadget)
{
    return object ? property.read(object) : property.readOnGadget(gadget);
}

bool writeProperty(const QMetaProperty &property, QObject *object, void *gadget,
                   const QVariant &value)
{
    return object ? property.write(object, value) : property.writeOnGadget(gadget, value);
}

QJsonValue toJsonValue(const QVariant &value, const QMetaProperty &property)
{
    // An enum was advertised as its key, so that is what it goes back out as.
    if (property.isEnumType()) {
        const char *key = property.enumerator().valueToKey(value.toInt());
        return key ? QJsonValue(QString::fromUtf8(key)) : QJsonValue(value.toInt());
    }

    const QMetaType type = value.metaType();
    if (type.flags().testFlag(QMetaType::IsGadget)) {
        if (const QMetaObject *meta = type.metaObject())
            return write(meta, value.constData());
    }
    return QJsonValue::fromVariant(value);
}

bool read(const QMetaObject *meta, QObject *object, void *gadget, const QJsonObject &json)
{
    if (!meta)
        return false;

    bool complete = true;
    // propertyOffset() skips QObject's own objectName, which is never part of
    // what the model was asked for.
    for (int i = meta->propertyOffset(); i < meta->propertyCount(); ++i) {
        const QMetaProperty property = meta->property(i);
        if (!property.isWritable())
            continue;
        const QJsonValue value = json.value(QString::fromUtf8(property.name()));
        if (value.isUndefined())
            continue;

        const QVariant converted = convert(value, property.metaType());
        if (!converted.isValid() || !writeProperty(property, object, gadget, converted))
            complete = false;
    }
    return complete;
}

QJsonObject writeAll(const QMetaObject *meta, const QObject *object, const void *gadget)
{
    QJsonObject json;
    for (int i = meta->propertyOffset(); i < meta->propertyCount(); ++i) {
        const QMetaProperty property = meta->property(i);
        if (!property.isReadable())
            continue;
        json.insert(QString::fromUtf8(property.name()),
                    toJsonValue(readProperty(property, object, gadget), property));
    }
    return json;
}

} // namespace

QVariant convert(const QJsonValue &value, QMetaType type)
{
    if (!type.isValid() || value.isUndefined())
        return {};

    if (type.flags().testFlag(QMetaType::IsEnumeration) && value.isString()) {
        const QMetaEnum enumerator = enumeratorOf(type);
        bool ok = false;
        const int key = enumerator.isValid()
                                ? enumerator.keyToValue(value.toString().toUtf8().constData(), &ok)
                                : 0;
        // An enum is its underlying integer; copying one in only works while
        // that is the plain `int` the meta-object system assumes.
        if (!ok || type.sizeOf() != qsizetype(sizeof(int)))
            return {};
        return QVariant(type, &key);
    }

    // A nested object is built through its own properties, which is how
    // MetaSchema described it.
    if (value.isObject() && type.flags().testFlag(QMetaType::IsGadget)) {
        QVariant nested(type, nullptr);
        if (!readInto(type.metaObject(), nested.data(), value.toObject()))
            return {};
        return nested;
    }

    QVariant variant = value.toVariant();
    if (variant.metaType() == type)
        return variant;
    if (!variant.convert(type))
        return {};
    return variant;
}

bool readInto(QObject *object, const QJsonObject &json)
{
    return object && read(object->metaObject(), object, nullptr, json);
}

bool readInto(const QMetaObject *meta, void *gadget, const QJsonObject &json)
{
    return gadget && read(meta, nullptr, gadget, json);
}

QJsonObject write(const QObject *object)
{
    return object ? writeAll(object->metaObject(), object, nullptr) : QJsonObject();
}

QJsonObject write(const QMetaObject *meta, const void *gadget)
{
    return meta && gadget ? writeAll(meta, nullptr, gadget) : QJsonObject();
}

} // namespace MetaJson
} // namespace Core
} // namespace QtOpenAi
