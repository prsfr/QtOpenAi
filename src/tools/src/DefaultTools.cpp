// SPDX-License-Identifier: MIT
#include "QtOpenAi/Tools/DefaultTools.h"

#include "QtOpenAi/Tools/FileTools.h"
#include "QtOpenAi/Tools/HttpTools.h"
#include "QtOpenAi/Tools/UtilityTools.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QMetaObject>

namespace QtOpenAi {
namespace Tools {

class DefaultToolsPrivate
{
public:
    FileTools *files = nullptr;
    HttpTools *http = nullptr;
    UtilityTools *utilities = nullptr;
    DefaultTools::ApprovalHandler approval;
    bool approveReads = false;
};

DefaultTools::DefaultTools(QObject *parent)
    : QObject(parent)
    , d_ptr(new DefaultToolsPrivate)
{ }

DefaultTools::~DefaultTools() = default;

void DefaultTools::setApprovalHandler(ApprovalHandler handler)
{
    Q_D(DefaultTools);
    d->approval = std::move(handler);
}

bool DefaultTools::hasApprovalHandler() const
{
    Q_D(const DefaultTools);
    return bool(d->approval);
}

void DefaultTools::setApproveReads(bool enabled)
{
    Q_D(DefaultTools);
    d->approveReads = enabled;
}

bool DefaultTools::approvesReads() const
{
    Q_D(const DefaultTools);
    return d->approveReads;
}

FileTools *DefaultTools::fileTools() const
{
    Q_D(const DefaultTools);
    return d->files;
}

HttpTools *DefaultTools::httpTools() const
{
    Q_D(const DefaultTools);
    return d->http;
}

UtilityTools *DefaultTools::utilityTools() const
{
    Q_D(const DefaultTools);
    return d->utilities;
}

QStringList DefaultTools::install(Client::ToolRegistry *registry, const ToolPolicy &policy)
{
    Q_D(DefaultTools);
    if (!registry)
        return {};

    QStringList installed;

    // Register `name` on `receiver`, wrapped in the approval gate when the call
    // can change something. The registry's own registerMethod() derives the
    // schema from the method, so it is used to *build* the tool definition and
    // the handler is then swapped for the gated one -- nothing about the tool
    // is written twice, and the gate cannot be forgotten for one of them.
    const auto add = [this, d, registry, &installed](QObject *receiver, const QString &name,
                                                     bool sideEffecting) {
        if (!registry->registerMethod(receiver, name))
            return;

        const bool gated = sideEffecting || d->approveReads;
        if (gated) {
            const Core::FunctionDefinition definition = [registry, &name]() {
                for (const Core::Tool &tool : registry->tools()) {
                    if (tool.function().name() == name)
                        return tool.function();
                }
                return Core::FunctionDefinition();
            }();

            registry->registerFunction(
                    definition.name(), definition.description(), definition.parameters(),
                    [this, d, receiver, name](const QJsonObject &arguments) -> QString {
                        if (d->approval && !d->approval(name, arguments)) {
                            Q_EMIT denied(name, arguments);
                            return QStringLiteral(
                                    "that was not approved; ask the user first, or do "
                                    "something else");
                        }
                        // Approved: dispatch through a second registry that
                        // knows only this method, so the argument mapping is
                        // the same one the ungated path would have used rather
                        // than a second implementation of it.
                        Client::ToolRegistry inner;
                        inner.registerMethod(receiver, name);
                        const Core::FunctionCall function(
                                name, QString::fromUtf8(QJsonDocument(arguments).toJson(
                                              QJsonDocument::Compact)));
                        return inner.invoke(Core::ToolCall(QStringLiteral("approved"), function))
                                .content();
                    });
        }
        installed.append(name);
    };

    if (policy.utilities) {
        if (!d->utilities)
            d->utilities = new UtilityTools(this);
        for (const QString &name : UtilityTools::toolNames())
            add(d->utilities, name, false);
    }

    if (policy.fileRead || policy.fileWrite) {
        FileSandbox sandbox = policy.sandbox;
        sandbox.setMaxBytes(policy.maxFileBytes);
        // A sandbox with no roots allows nothing, so a tool built on one could
        // only ever refuse. Not installing it is the honest outcome, and the
        // returned list is how the caller finds out they misconfigured it.
        if (!sandbox.isEmpty()) {
            if (!d->files)
                d->files = new FileTools(this);
            d->files->setSandbox(sandbox);

            if (policy.fileRead) {
                for (const QString &name : FileTools::readingTools())
                    add(d->files, name, false);
            }
            // Two switches for the one irreversible power: the policy has to
            // ask for writing *and* the sandbox has to permit it. Either alone
            // is not enough, so neither can turn it on by accident.
            if (policy.fileWrite && !sandbox.isReadOnly()) {
                for (const QString &name : FileTools::writingTools())
                    add(d->files, name, true);
            }
        }
    }

    if (policy.httpGet && !policy.allowedHosts.isEmpty()) {
        if (!d->http)
            d->http = new HttpTools(this);
        d->http->setAllowedHosts(policy.allowedHosts);
        d->http->setRequiresHttps(policy.requireHttps);
        d->http->setMaxBytes(policy.maxResponseBytes);
        d->http->setTimeoutMs(policy.httpTimeoutMs);
        // Side-effecting in the sense that matters: it leaves the process, from
        // inside whatever network this application is running in.
        add(d->http, QStringLiteral("http_get"), true);
    }

    return installed;
}

} // namespace Tools
} // namespace QtOpenAi
