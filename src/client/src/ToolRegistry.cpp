// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ToolRegistry.h"

#include <QtOpenAi/Core/MetaSchema.h>
#include <QtOpenAi/Core/SchemaValidator.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QMetaEnum>
#include <QtCore/QMetaMethod>
#include <QtCore/QMetaObject>
#include <QtCore/QPointer>
#include <QtCore/QVarLengthArray>
#include <QtCore/QVariantMap>

namespace QtOpenAi {
namespace Client {

namespace {

// QMetaMethod::invoke takes at most ten arguments, which is the ceiling on a
// method usable as a tool.
constexpr int kMaxArguments = 10;

// Serialise a small JSON error object for use as a tool-result payload. The
// details list carries one entry per rejected constraint, so a model reading
// the result learns exactly what to change.
QString makeErrorPayload(const QString &message, const QStringList &details = QStringList())
{
    QJsonObject error;
    error.insert(QStringLiteral("error"), message);
    if (!details.isEmpty())
        error.insert(QStringLiteral("details"), QJsonArray::fromStringList(details));
    return QString::fromUtf8(QJsonDocument(error).toJson(QJsonDocument::Compact));
}

// The invokable method of that name, or an invalid QMetaMethod.
QMetaMethod findMethod(const QObject *receiver, const QString &name)
{
    const QMetaObject *meta = receiver->metaObject();
    const QByteArray wanted = name.toUtf8();
    for (int i = 0; i < meta->methodCount(); ++i) {
        const QMetaMethod method = meta->method(i);
        if (method.name() == wanted)
            return method;
    }
    return {};
}

// Whether the method wants the arguments object as a whole rather than
// unpacked into its parameters.
bool takesWholeArguments(const QMetaMethod &method)
{
    if (method.parameterCount() != 1)
        return false;
    const int id = method.parameterMetaType(0).id();
    return id == QMetaType::QJsonObject || id == QMetaType::QVariantMap;
}

// Turn one JSON value into the C++ type a parameter declares. Enums arrive as
// their key, because that is how MetaSchema advertises them.
bool coerce(const QJsonValue &value, QMetaType type, QVariant *out)
{
    if (type.flags().testFlag(QMetaType::IsEnumeration) && value.isString()) {
        const QMetaObject *meta = type.metaObject();
        if (!meta)
            return false;
        const QByteArray enumerator = QByteArray(type.name()).split(':').last();
        const int index = meta->indexOfEnumerator(enumerator.constData());
        if (index < 0)
            return false;
        bool ok = false;
        const int key
                = meta->enumerator(index).keyToValue(value.toString().toUtf8().constData(), &ok);
        // An enum is its underlying integer; copying one in only works while
        // that is the plain `int` the meta-object system assumes.
        if (!ok || type.sizeOf() != qsizetype(sizeof(int)))
            return false;
        *out = QVariant(type, &key);
        return true;
    }

    QVariant variant = value.toVariant();
    if (variant.metaType() == type) {
        *out = variant;
        return true;
    }
    if (!variant.convert(type))
        return false;
    *out = variant;
    return true;
}

// A return value as the tool result the model receives. Structured returns are
// serialised rather than stringified, so a method can answer with JSON.
QString stringify(const QVariant &value)
{
    switch (value.metaType().id()) {
    case QMetaType::QJsonObject:
        return QString::fromUtf8(
                QJsonDocument(value.toJsonObject()).toJson(QJsonDocument::Compact));
    case QMetaType::QJsonArray:
        return QString::fromUtf8(QJsonDocument(value.toJsonArray()).toJson(QJsonDocument::Compact));
    case QMetaType::QVariantMap:
        return QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(value.toMap()))
                                         .toJson(QJsonDocument::Compact));
    case QMetaType::QVariantList:
        return QString::fromUtf8(QJsonDocument(QJsonArray::fromVariantList(value.toList()))
                                         .toJson(QJsonDocument::Compact));
    default:
        break;
    }
    return value.toString();
}

// Call the method with the model's arguments, writing the result. Returns the
// reason it could not be called, or an empty string on success.
QString dispatch(QObject *receiver, const QMetaMethod &method, const QJsonObject &arguments,
                 QString *result)
{
    const QString name = QString::fromUtf8(method.name());
    if (method.parameterCount() > kMaxArguments) {
        return QStringLiteral("method '%1' takes more than %2 arguments")
                .arg(name, QString::number(kMaxArguments));
    }

    // The values must outlive the call: a QGenericArgument only points at them.
    QVarLengthArray<QVariant, kMaxArguments> values;
    if (takesWholeArguments(method)) {
        values.append(method.parameterMetaType(0).id() == QMetaType::QJsonObject
                              ? QVariant::fromValue(arguments)
                              : QVariant(arguments.toVariantMap()));
    } else {
        const QList<QByteArray> names = method.parameterNames();
        for (int i = 0; i < method.parameterCount(); ++i) {
            const QString parameter = QString::fromUtf8(names.value(i));
            if (!arguments.contains(parameter)) {
                return QStringLiteral("method '%1' is missing argument '%2'").arg(name, parameter);
            }
            QVariant value;
            const QMetaType type = method.parameterMetaType(i);
            if (!coerce(arguments.value(parameter), type, &value)) {
                return QStringLiteral("argument '%1' of method '%2' is not a %3")
                        .arg(parameter, name, QString::fromUtf8(type.name()));
            }
            values.append(value);
        }
    }

    QVarLengthArray<QGenericArgument, kMaxArguments> generic;
    for (QVariant &value : values)
        generic.append(QGenericArgument(value.metaType().name(), value.constData()));
    generic.resize(kMaxArguments); // Unused slots stay default-constructed.

    // Receiving the return value in its own type keeps methods answering with
    // something other than QString usable.
    const QMetaType returnType = method.returnMetaType();
    const bool returns = returnType.isValid() && returnType.id() != QMetaType::Void;
    QVariant returned = returns ? QVariant(returnType, nullptr) : QVariant();
    const QGenericReturnArgument slot
            = returns ? QGenericReturnArgument(returnType.name(), returned.data())
                      : QGenericReturnArgument();

    const bool ok = method.invoke(receiver, Qt::DirectConnection, slot, generic[0], generic[1],
                                  generic[2], generic[3], generic[4], generic[5], generic[6],
                                  generic[7], generic[8], generic[9]);
    if (!ok)
        return QStringLiteral("failed to invoke method '%1'").arg(name);

    *result = returns ? stringify(returned) : QString();
    return {};
}

} // namespace

class ToolRegistryPrivate
{
public:
    struct Entry
    {
        Core::Tool tool;
        ToolRegistry::Handler handler; // functor dispatch (may be null)
        QPointer<QObject> receiver;    // meta-object dispatch (may be null)
        QMetaMethod method;            // resolved once, at registration
    };

    // Insertion-ordered storage keyed by tool name.
    QList<QString> order;
    QHash<QString, Entry> entries;
    bool validateArguments = false;

    void put(const Entry &entry)
    {
        const QString name = entry.tool.function().name();
        if (!entries.contains(name))
            order.append(name);
        entries.insert(name, entry);
    }
};

ToolRegistry::ToolRegistry(QObject *parent)
    : QObject(parent)
    , d_ptr(new ToolRegistryPrivate)
{ }

ToolRegistry::~ToolRegistry() = default;

void ToolRegistry::registerTool(const Core::Tool &tool, Handler handler)
{
    Q_D(ToolRegistry);
    ToolRegistryPrivate::Entry entry;
    entry.tool = tool;
    entry.handler = std::move(handler);
    d->put(entry);
}

void ToolRegistry::registerFunction(const QString &name, const QString &description,
                                    const QJsonObject &parameters, Handler handler)
{
    registerTool(Core::Tool::function(name, description, parameters), std::move(handler));
}

bool ToolRegistry::registerMethod(const Core::Tool &tool, QObject *receiver, const QString &method)
{
    Q_D(ToolRegistry);
    if (!receiver)
        return false;

    // Resolve the method now, so registration fails fast instead of at call
    // time -- and so dispatch knows the signature it has to fill in.
    const QMetaMethod resolved = findMethod(receiver, method);
    if (!resolved.isValid())
        return false;

    ToolRegistryPrivate::Entry entry;
    entry.tool = tool;
    entry.receiver = receiver;
    entry.method = resolved;
    d->put(entry);
    return true;
}

bool ToolRegistry::registerMethod(QObject *receiver, const QString &method,
                                  const QString &description)
{
    if (!receiver)
        return false;

    QJsonObject parameters = Core::MetaSchema::fromMethod(receiver->metaObject(), method);
    // The description belongs on the function, not on its parameters object;
    // MetaSchema puts the Q_CLASSINFO annotation there because it has nowhere
    // else to leave it.
    const QString documented = parameters.take(QStringLiteral("description")).toString();

    return registerMethod(Core::Tool::function(method,
                                               description.isEmpty() ? documented : description,
                                               parameters),
                          receiver, method);
}

void ToolRegistry::setValidateArguments(bool enabled)
{
    Q_D(ToolRegistry);
    d->validateArguments = enabled;
}

bool ToolRegistry::validatesArguments() const
{
    Q_D(const ToolRegistry);
    return d->validateArguments;
}

bool ToolRegistry::unregister(const QString &name)
{
    Q_D(ToolRegistry);
    if (!d->entries.remove(name))
        return false;
    d->order.removeAll(name);
    return true;
}

void ToolRegistry::clear()
{
    Q_D(ToolRegistry);
    d->entries.clear();
    d->order.clear();
}

bool ToolRegistry::contains(const QString &name) const
{
    Q_D(const ToolRegistry);
    return d->entries.contains(name);
}

QStringList ToolRegistry::toolNames() const
{
    Q_D(const ToolRegistry);
    return d->order;
}

QList<Core::Tool> ToolRegistry::tools() const
{
    Q_D(const ToolRegistry);
    QList<Core::Tool> result;
    result.reserve(d->order.size());
    for (const QString &name : d->order)
        result.append(d->entries.value(name).tool);
    return result;
}

Core::Message ToolRegistry::invoke(const Core::ToolCall &call)
{
    Q_D(ToolRegistry);
    const QString name = call.function().name();
    const QString id = call.id();

    auto it = d->entries.constFind(name);
    if (it == d->entries.constEnd()) {
        Q_EMIT unknownTool(id, name);
        const QString payload = makeErrorPayload(QStringLiteral("unknown tool: %1").arg(name));
        Q_EMIT toolFailed(id, name, payload);
        return Core::Message::toolResult(id, payload);
    }

    const ToolRegistryPrivate::Entry &entry = it.value();
    const QJsonObject arguments = call.function().argumentsObject();

    if (d->validateArguments) {
        const QStringList errors
                = Core::SchemaValidator::validate(entry.tool.function().parameters(), arguments);
        if (!errors.isEmpty()) {
            const QString payload = makeErrorPayload(
                    QStringLiteral("invalid arguments for tool '%1'").arg(name), errors);
            Q_EMIT argumentsRejected(id, name, errors);
            Q_EMIT toolFailed(id, name, payload);
            return Core::Message::toolResult(id, payload);
        }
    }

    QString result;
    QString failure;

    if (entry.handler)
        result = entry.handler(arguments);
    else if (entry.receiver)
        failure = dispatch(entry.receiver.data(), entry.method, arguments, &result);
    else
        failure = QStringLiteral("tool '%1' has no handler").arg(name);

    if (!failure.isEmpty()) {
        const QString payload = makeErrorPayload(failure);
        Q_EMIT toolFailed(id, name, payload);
        return Core::Message::toolResult(id, payload);
    }

    Q_EMIT toolInvoked(id, name, result);
    return Core::Message::toolResult(id, result);
}

QList<Core::Message> ToolRegistry::invokeAll(const QList<Core::ToolCall> &calls)
{
    QList<Core::Message> messages;
    messages.reserve(calls.size());
    for (const Core::ToolCall &call : calls)
        messages.append(invoke(call));
    return messages;
}

} // namespace Client
} // namespace QtOpenAi
