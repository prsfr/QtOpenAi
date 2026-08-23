// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ToolRegistry.h"

#include <QtOpenAi/Core/MetaJson.h>
#include <QtOpenAi/Core/MetaSchema.h>
#include <QtOpenAi/Core/SchemaValidator.h>

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QMetaMethod>
#include <QtCore/QMetaObject>
#include <QtCore/QPointer>
#include <QtCore/QVarLengthArray>
#include <QtCore/QVariantMap>

namespace QtOpenAi {
namespace Client {

namespace {

// Serialise a small JSON error object for use as a tool-result payload. The
// details list carries one entry per rejected constraint, so a model reading
// the result learns exactly what to change.
QString makeErrorPayload(const QString &message, const QStringList &details = QStringList())
{
    QJsonObject error;
    error.insert(QStringLiteral("error"), message);
    if (!details.isEmpty())
        error.insert(QStringLiteral("details"), QJsonArray::fromStringList(details));
    return Core::detail::compactJsonText(error);
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

// A return value as the tool result the model receives. Structured returns are
// serialised rather than stringified, so a method can answer with JSON.
QString stringify(const QVariant &value)
{
    switch (value.metaType().id()) {
    case QMetaType::QJsonObject:
        return Core::detail::compactJsonText(value.toJsonObject());
    case QMetaType::QJsonArray:
        return Core::detail::compactJsonText(value.toJsonArray());
    case QMetaType::QVariantMap:
        return Core::detail::compactJsonText(QJsonObject::fromVariantMap(value.toMap()));
    case QMetaType::QVariantList:
        return Core::detail::compactJsonText(QJsonArray::fromVariantList(value.toList()));
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

    // The values must outlive the call: what is handed over are pointers.
    QVarLengthArray<QVariant, 8> values;
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
            // Same conversion the model was promised by the schema: an enum
            // named by its key, a nested object built from its properties.
            const QMetaType type = method.parameterMetaType(i);
            const QVariant value = Core::MetaJson::convert(arguments.value(parameter), type);
            if (!value.isValid()) {
                return QStringLiteral("argument '%1' of method '%2' is not a %3")
                        .arg(parameter, name, QString::fromUtf8(type.name()));
            }
            values.append(value);
        }
    }

    // Receiving the return value in its own type keeps methods answering with
    // something other than QString usable.
    const QMetaType returnType = method.returnMetaType();
    const bool returns = returnType.isValid() && returnType.id() != QMetaType::Void;
    QVariant returned = returns ? QVariant(returnType, nullptr) : QVariant();

    // QMetaMethod::invoke() would re-derive each argument's type from a type
    // *name*, which an enum declared with Q_ENUM is not reliably registered
    // under -- Qt 6.11 rejects the call that Qt 6.4 accepted. Every value here
    // was already converted to the QMetaType the meta-object itself reports for
    // that parameter, so the call goes straight to the metacall invoke() would
    // reach after its own checks.
    QVarLengthArray<void *, 9> argv;
    argv.append(returns ? returned.data() : nullptr);
    for (QVariant &value : values)
        argv.append(value.data());

    if (QMetaObject::metacall(receiver, QMetaObject::InvokeMetaMethod, method.methodIndex(),
                              argv.data())
        >= 0) {
        return QStringLiteral("failed to invoke method '%1'").arg(name);
    }

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

    // A description whose key names nothing is legal C++ and silent: the model
    // is simply handed a tool it is told nothing about. Only the meta-object
    // knows the real names, and this is the moment it and the annotations are
    // both in hand -- so say so here, rather than waiting for someone to write
    // the assertion. Filtered to this method, because the rest of the class is
    // not what the caller just asked about.
    const QString prefix = QStringLiteral("doc:") + method;
    const QStringList dangling = Core::MetaSchema::danglingAnnotations(receiver->metaObject());
    for (const QString &annotation : dangling) {
        if (annotation == prefix || annotation.startsWith(prefix + QLatin1Char(':'))) {
            qWarning("QtOpenAi: %s describes nothing on %s -- the model will be given this tool "
                     "with that description missing. A renamed method or argument?",
                     qUtf8Printable(annotation), receiver->metaObject()->className());
        }
    }

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
