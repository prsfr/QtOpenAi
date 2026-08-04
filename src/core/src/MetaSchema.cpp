// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/MetaSchema.h"

#include <QtCore/QJsonArray>
#include <QtCore/QMetaEnum>
#include <QtCore/QMetaMethod>
#include <QtCore/QMetaProperty>
#include <QtCore/QStringList>

namespace QtOpenAi {
namespace Core {
namespace MetaSchema {

namespace {

constexpr QLatin1String kType("type");
constexpr QLatin1String kItems("items");
constexpr QLatin1String kProperties("properties");
constexpr QLatin1String kRequired("required");
constexpr QLatin1String kDescription("description");
constexpr QLatin1String kAdditionalProperties("additionalProperties");

// The Q_CLASSINFO key a description lives under. "doc" for the object itself,
// "doc:member" for one of its properties, "doc:method:argument" for one
// argument -- so an annotation reads as the path to what it describes.
QString docKey(const QString &path = {})
{
    return path.isEmpty() ? QStringLiteral("doc") : QStringLiteral("doc:") + path;
}

QString classInfo(const QMetaObject *meta, const QString &key)
{
    if (!meta)
        return {};
    const int index = meta->indexOfClassInfo(key.toUtf8().constData());
    return index < 0 ? QString() : QString::fromUtf8(meta->classInfo(index).value());
}

QJsonObject typed(QLatin1String type)
{
    QJsonObject schema;
    schema.insert(kType, QString(type));
    return schema;
}

// A string whose shape has a name in JSON Schema. `format` is advisory -- a
// validator may ignore it -- but it is the standard way to say "this is a date"
// or "this is a URL", and it costs one word where prose would cost a sentence.
QJsonObject formatted(QLatin1String format)
{
    QJsonObject schema = typed(QLatin1String("string"));
    schema.insert(QStringLiteral("format"), QString(format));
    return schema;
}

// An integer the type says cannot be negative.
QJsonObject atLeast(qint64 minimum)
{
    QJsonObject schema = typed(QLatin1String("integer"));
    schema.insert(QStringLiteral("minimum"), double(minimum));
    return schema;
}

// An integer whose whole range the type knows.
QJsonObject bounded(qint64 minimum, qint64 maximum)
{
    QJsonObject schema = atLeast(minimum);
    schema.insert(QStringLiteral("maximum"), double(maximum));
    return schema;
}

QJsonObject arrayOf(const QJsonObject &items)
{
    QJsonObject schema = typed(QLatin1String("array"));
    if (!items.isEmpty())
        schema.insert(kItems, items);
    return schema;
}

// An enum is a closed set of names. Describing it as a string with an `enum`
// constraint tells the model which values exist, where a bare integer would
// not.
QJsonObject fromEnum(const QMetaEnum &metaEnum)
{
    QJsonArray keys;
    for (int i = 0; i < metaEnum.keyCount(); ++i)
        keys.append(QString::fromUtf8(metaEnum.key(i)));

    QJsonObject schema = typed(QLatin1String("string"));
    if (!keys.isEmpty())
        schema.insert(QStringLiteral("enum"), keys);
    return schema;
}

// Assemble an object schema from name/schema pairs, in declaration order.
// Everything is required and nothing else is accepted -- see the header for why.
QJsonObject objectOf(const QList<QPair<QString, QJsonObject>> &members)
{
    QJsonObject properties;
    QJsonArray required;
    for (const auto &member : members) {
        properties.insert(member.first, member.second);
        required.append(member.first);
    }

    QJsonObject schema = typed(QLatin1String("object"));
    schema.insert(kProperties, properties);
    if (!required.isEmpty())
        schema.insert(kRequired, required);
    schema.insert(kAdditionalProperties, false);
    return schema;
}

void describe(QJsonObject &schema, const QString &description)
{
    if (!description.isEmpty())
        schema.insert(kDescription, description);
}

} // namespace

QJsonObject fromMetaType(QMetaType type)
{
    switch (type.id()) {
    case QMetaType::QString:
    case QMetaType::QByteArray:
    case QMetaType::Char:
    case QMetaType::QChar:
        return typed(QLatin1String("string"));
    case QMetaType::Bool:
        return typed(QLatin1String("boolean"));
    // The bounds come from the type, and they are facts the *name* could never
    // carry: a model asked for a `quint8` has no way to know it may not answer
    // 300, and SchemaValidator enforces minimum/maximum, so saying so here also
    // means a wrong value is rejected before it ever reaches the method.
    //
    // Only bounds a double represents exactly are stated. A qint64's extremes
    // are not, and a limit that is off by a few hundred is worse than no limit:
    // it would reject a value the method accepts.
    case QMetaType::SChar:
        return bounded(-128, 127);
    case QMetaType::UChar:
        return bounded(0, 255);
    case QMetaType::Short:
        return bounded(-32768, 32767);
    case QMetaType::UShort:
        return bounded(0, 65535);
    case QMetaType::Int:
        return bounded(-2147483648LL, 2147483647LL);
    case QMetaType::UInt:
        return bounded(0, 4294967295LL);
    case QMetaType::ULong:
    case QMetaType::ULongLong:
        // Unsigned, so never negative; the upper end is past what a double
        // states exactly, and is left unsaid rather than stated wrongly.
        return atLeast(0);
    case QMetaType::Long:
    case QMetaType::LongLong:
        return typed(QLatin1String("integer"));
    case QMetaType::Double:
    case QMetaType::Float:
        return typed(QLatin1String("number"));
    case QMetaType::QStringList:
        return arrayOf(typed(QLatin1String("string")));
    case QMetaType::QVariantList:
    case QMetaType::QJsonArray:
        return arrayOf({});
    case QMetaType::QVariantMap:
    case QMetaType::QVariantHash:
    case QMetaType::QJsonObject:
        return typed(QLatin1String("object"));
    case QMetaType::QDate:
        return formatted(QLatin1String("date"));
    case QMetaType::QDateTime:
        return formatted(QLatin1String("date-time"));
    case QMetaType::QTime:
        return formatted(QLatin1String("time"));
    // A URL and a UUID are strings with a shape, and JSON Schema has a name for
    // each. Both were previously unknown types, so they reached the model as an
    // empty schema -- no type at all, which accepts anything.
    case QMetaType::QUrl:
        return formatted(QLatin1String("uri"));
    case QMetaType::QUuid:
        return formatted(QLatin1String("uuid"));
    default:
        break;
    }

    // Enums and gadgets are only knowable through the type's own meta-object.
    if (type.flags().testFlag(QMetaType::IsEnumeration)) {
        const QMetaObject *meta = type.metaObject();
        if (meta) {
            const QByteArray name = QByteArray(type.name()).split(':').last();
            const int index = meta->indexOfEnumerator(name.constData());
            if (index >= 0)
                return fromEnum(meta->enumerator(index));
        }
        // A registered enum whose enumerator cannot be located is still an
        // integer on the wire.
        return typed(QLatin1String("integer"));
    }

    if (const QMetaObject *meta = type.metaObject())
        return fromMetaObject(meta);

    // Not a type this mapping describes: an empty schema constrains nothing,
    // which beats guessing.
    return {};
}

QJsonObject fromMetaObject(const QMetaObject *meta)
{
    if (!meta)
        return {};

    QList<QPair<QString, QJsonObject>> members;
    // propertyOffset() skips QObject's own `objectName`, which is never part of
    // what the caller means to describe.
    for (int i = meta->propertyOffset(); i < meta->propertyCount(); ++i) {
        const QMetaProperty property = meta->property(i);
        if (!property.isReadable())
            continue;

        QJsonObject schema = property.isEnumType() ? fromEnum(property.enumerator())
                                                   : fromMetaType(property.metaType());
        const QString name = QString::fromUtf8(property.name());
        describe(schema, classInfo(meta, docKey(name)));
        members.append({name, schema});
    }

    QJsonObject schema = objectOf(members);
    describe(schema, classInfo(meta, docKey()));
    return schema;
}

QStringList danglingAnnotations(const QMetaObject *meta)
{
    if (!meta)
        return {};

    // What fromMetaObject() would emit -- inherited properties are not in it,
    // so an annotation for one describes nothing this ever produces.
    QStringList properties;
    for (int i = meta->propertyOffset(); i < meta->propertyCount(); ++i)
        properties.append(QString::fromUtf8(meta->property(i).name()));

    const QString prefix = docKey() + QLatin1Char(':');
    QStringList dangling;
    for (int i = 0; i < meta->classInfoCount(); ++i) {
        const QString key = QString::fromUtf8(meta->classInfo(i).name());
        // "doc" describes the class, and always exists.
        if (!key.startsWith(prefix))
            continue;

        const QStringList path = key.mid(prefix.size()).split(QLatin1Char(':'));
        const QString member = path.value(0);

        QMetaMethod method;
        for (int m = 0; m < meta->methodCount(); ++m) {
            if (meta->method(m).name() == member.toUtf8()) {
                method = meta->method(m);
                break;
            }
        }

        if (path.size() == 1) {
            if (!properties.contains(member) && !method.isValid())
                dangling.append(key);
            continue;
        }

        // doc:<method>:<argument> -- the method has to exist and to have that
        // parameter, under the name moc recorded or the arg%1 fallback.
        if (path.size() > 2 || !method.isValid()) {
            dangling.append(key);
            continue;
        }

        const QList<QByteArray> names = method.parameterNames();
        bool found = false;
        for (int p = 0; p < method.parameterCount(); ++p) {
            const QString name = names.value(p).isEmpty() ? QStringLiteral("arg%1").arg(p)
                                                          : QString::fromUtf8(names.at(p));
            if (name == path.at(1)) {
                found = true;
                break;
            }
        }
        if (!found)
            dangling.append(key);
    }
    return dangling;
}

QJsonObject fromMethod(const QMetaObject *meta, const QString &method)
{
    if (!meta)
        return {};

    const QByteArray wanted = method.toUtf8();
    for (int i = 0; i < meta->methodCount(); ++i) {
        const QMetaMethod signature = meta->method(i);
        if (signature.name() != wanted)
            continue;

        const QList<QByteArray> names = signature.parameterNames();
        QList<QPair<QString, QJsonObject>> members;
        for (int p = 0; p < signature.parameterCount(); ++p) {
            QJsonObject schema = fromMetaType(signature.parameterMetaType(p));
            // moc keeps the parameter names from the declaration; a signature
            // written without them still needs something addressable.
            const QString name = names.value(p).isEmpty() ? QStringLiteral("arg%1").arg(p)
                                                          : QString::fromUtf8(names.at(p));
            describe(schema, classInfo(meta, docKey(method + QLatin1Char(':') + name)));
            members.append({name, schema});
        }

        QJsonObject schema = objectOf(members);
        describe(schema, classInfo(meta, docKey(method)));
        return schema;
    }

    return {};
}

} // namespace MetaSchema
} // namespace Core
} // namespace QtOpenAi
