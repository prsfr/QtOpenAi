// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Core/Message.h>
#include <QtOpenAi/Core/Tool.h>
#include <QtOpenAi/Core/ToolCall.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>

#include <functional>

namespace QtOpenAi {
namespace Client {

class ToolRegistryPrivate;

// A registry that maps model tool calls back onto local C++ code.
//
// Two dispatch styles are supported, both integrated with Qt's meta-object
// system:
//
//  * A std::function handler taking the parsed JSON arguments and returning
//    the tool result as a string.
//  * A QObject slot invoked by name via QMetaObject::invokeMethod. The slot
//    must have the signature `QString slot(const QJsonObject &)` (or accept a
//    QVariantMap). This lets any QObject expose Q_INVOKABLE methods as tools.
//
// Results are delivered both as return values (invoke / invokeAll) and via
// signals, so a UI can react asynchronously to tool execution.
class QTOPENAI_CLIENT_EXPORT ToolRegistry : public QObject
{
    Q_OBJECT
public:
    // Handler receiving the decoded arguments object and returning the result
    // that will be sent back to the model as a tool message.
    using Handler = std::function<QString(const QJsonObject &arguments)>;

    explicit ToolRegistry(QObject *parent = nullptr);
    ~ToolRegistry() override;

    // Register a tool backed by a std::function handler.
    void registerTool(const Core::Tool &tool, Handler handler);

    // Convenience overload building the Tool definition inline.
    void registerFunction(const QString &name,
                          const QString &description,
                          const QJsonObject &parameters,
                          Handler handler);

    // Register a tool dispatched to a QObject slot via the meta-object system.
    // The named method is resolved and invoked with QMetaObject::invokeMethod.
    // Returns false if no matching invokable method exists on the receiver.
    bool registerMethod(const Core::Tool &tool,
                        QObject *receiver,
                        const QString &method);

    // Remove a registered tool. Returns true if it existed.
    bool unregister(const QString &name);
    void clear();

    bool contains(const QString &name) const;
    QStringList toolNames() const;

    // The tool definitions to advertise in a ChatCompletionRequest.
    QList<Core::Tool> tools() const;

    // Execute a single tool call, returning a tool-result Message ready to be
    // appended to the conversation. On error, the message content carries a
    // JSON error object and toolFailed() is emitted.
    Core::Message invoke(const Core::ToolCall &call);

    // Execute every tool call in order, returning one result message per call.
    QList<Core::Message> invokeAll(const QList<Core::ToolCall> &calls);

Q_SIGNALS:
    // Emitted after a tool call was dispatched successfully.
    void toolInvoked(const QString &id, const QString &name, const QString &result);
    // Emitted when a tool handler reported/raised an error.
    void toolFailed(const QString &id, const QString &name, const QString &error);
    // Emitted when a call referenced a name that is not registered.
    void unknownTool(const QString &id, const QString &name);

private:
    Q_DECLARE_PRIVATE(ToolRegistry)
    QScopedPointer<ToolRegistryPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
