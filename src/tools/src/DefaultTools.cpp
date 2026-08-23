// SPDX-License-Identifier: MIT
#include "QtOpenAi/Tools/DefaultTools.h"

#include "QtOpenAi/Tools/FileTools.h"
#include "QtOpenAi/Tools/HttpTools.h"
#include "QtOpenAi/Tools/UtilityTools.h"

#include "JsonHelpers_p.h"

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

    // Where an approved call is actually dispatched from. A gated tool is
    // registered on the caller's registry with a handler that asks first, so
    // the receiver and its argument mapping have to live somewhere else -- and
    // that somewhere is built once, at install time, rather than rebuilt inside
    // the handler on every approval. Rebuilding it there meant re-scanning the
    // receiver's meta-object and re-deriving the method's JSON schema for each
    // call the user said yes to, and re-issuing registerMethod()'s
    // dangling-annotation warnings with it.
    Client::ToolRegistry *dispatch = nullptr;
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
            // The tool registerMethod() just derived from the method, so the
            // gated version is described to the model exactly as the ungated
            // one would have been.
            const Core::Tool tool = [registry, &name]() {
                for (const Core::Tool &candidate : registry->tools()) {
                    if (candidate.function().name() == name)
                        return candidate;
                }
                return Core::Tool();
            }();
            const Core::FunctionDefinition definition = tool.function();

            // The same method again on the dispatch registry, so an approved
            // call goes through the argument mapping the ungated path would
            // have used rather than a second implementation of it. Registered
            // here and not in the handler: it is the same registration every
            // time, and doing it per call charged every approval for a
            // meta-object scan, a fresh schema derived from it, and another
            // round of registerMethod()'s dangling-annotation warnings. It is
            // also the overload taking the definition already in hand, so that
            // derivation does not happen a second time even here.
            if (!d->dispatch)
                d->dispatch = new Client::ToolRegistry(this);
            d->dispatch->registerMethod(tool, receiver, name);

            registry->registerFunction(
                    definition.name(), definition.description(), definition.parameters(),
                    [this, d, name](const QJsonObject &arguments) -> QString {
                        if (d->approval && !d->approval(name, arguments)) {
                            Q_EMIT denied(name, arguments);
                            return QStringLiteral(
                                    "that was not approved; ask the user first, or do "
                                    "something else");
                        }
                        const Core::FunctionCall function(name,
                                                          Core::detail::compactJsonText(arguments));
                        return d->dispatch
                                ->invoke(Core::ToolCall(QStringLiteral("approved"), function))
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
