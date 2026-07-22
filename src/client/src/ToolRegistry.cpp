// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ToolRegistry.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QMetaMethod>
#include <QtCore/QMetaObject>
#include <QtCore/QPointer>
#include <QtCore/QVariantMap>

namespace QtOpenAi {
namespace Client {

namespace {

// Serialise a small JSON error object for use as a tool-result payload.
QString makeErrorPayload(const QString &message)
{
    QJsonObject error;
    error.insert(QStringLiteral("error"), message);
    return QString::fromUtf8(QJsonDocument(error).toJson(QJsonDocument::Compact));
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
        QString method;                // slot name for meta-object dispatch
    };

    // Insertion-ordered storage keyed by tool name.
    QList<QString> order;
    QHash<QString, Entry> entries;

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

    // Verify the receiver exposes an invokable method with the given name so
    // that registration fails fast instead of at call time.
    const QMetaObject *meta = receiver->metaObject();
    bool found = false;
    for (int i = 0; i < meta->methodCount(); ++i) {
        if (meta->method(i).name() == method.toUtf8()) {
            found = true;
            break;
        }
    }
    if (!found)
        return false;

    ToolRegistryPrivate::Entry entry;
    entry.tool = tool;
    entry.receiver = receiver;
    entry.method = method;
    d->put(entry);
    return true;
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

    QString result;
    QString failure;

    if (entry.handler) {
        result = entry.handler(arguments);
    } else if (entry.receiver) {
        // Dispatch through the meta-object system by method name.
        bool ok = QMetaObject::invokeMethod(
                entry.receiver.data(), entry.method.toUtf8().constData(), Qt::DirectConnection,
                Q_RETURN_ARG(QString, result), Q_ARG(QJsonObject, arguments));
        if (!ok) {
            // Retry with a QVariantMap argument for slots preferring that type.
            QVariantMap variantArgs = arguments.toVariantMap();
            ok = QMetaObject::invokeMethod(entry.receiver.data(), entry.method.toUtf8().constData(),
                                           Qt::DirectConnection, Q_RETURN_ARG(QString, result),
                                           Q_ARG(QVariantMap, variantArgs));
        }
        if (!ok)
            failure = QStringLiteral("failed to invoke method '%1'").arg(entry.method);
    } else {
        failure = QStringLiteral("tool '%1' has no handler").arg(name);
    }

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
