// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ToolRegistry.h>
#include <QtOpenAi/Tools/FileSandbox.h>
#include <QtOpenAi/Tools/GlobalTools.h>

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QStringList>

#include <functional>

namespace QtOpenAi {
namespace Tools {

class DefaultToolsPrivate;

// Which of the ready-made tools an application is willing to hand to a model.
//
// Everything here is off. That is the point: a policy nobody filled in installs
// nothing, so forgetting to configure this cannot be the thing that gives a
// model your filesystem.
//
//     ToolPolicy policy;
//     policy.utilities = true;
//     policy.fileRead = true;
//     policy.sandbox = FileSandbox({docsPath});
//
// The file powers are separate flags rather than a level, because reading a
// directory and rewriting it are different decisions and one does not imply the
// other. `fileWrite` additionally needs a sandbox that is not read-only -- two
// switches for the one irreversible power, so it cannot be turned on by
// accident.
struct QTOPENAI_TOOLS_EXPORT ToolPolicy
{
    // current_time, calculate, uuid. No policy needed; they cannot reach
    // anything.
    bool utilities = false;

    // read_file, list_directory, file_exists -- confined to `sandbox`.
    bool fileRead = false;
    // write_file. Needs `sandbox.isReadOnly() == false` as well.
    bool fileWrite = false;
    FileSandbox sandbox;

    // http_get, confined to `allowedHosts`. With no hosts, nothing installs:
    // an HTTP tool with an empty allow-list can do nothing but refuse, and
    // advertising it to the model would only waste tokens.
    bool httpGet = false;
    QStringList allowedHosts;
    bool requireHttps = true;
    qint64 maxResponseBytes = 256 * 1024;
    int httpTimeoutMs = 10000;

    // Largest file that may be read or written. Mirrors sandbox.maxBytes() and
    // is applied to it at install time.
    qint64 maxFileBytes = 1024 * 1024;
};

// Installs the ready-made tools into a ToolRegistry, according to a policy.
//
//     DefaultTools tools;
//     tools.setApprovalHandler([this](const QString &name, const QJsonObject &args) {
//         return askTheUser(name, args);
//     });
//     tools.install(&registry, policy);
//
// A model that can call a tool is a model that will call it, steered by
// whatever text is in its context -- which may include text an attacker wrote.
// So the two questions this class answers are "what is this application willing
// to allow at all", which is the policy, and "does the user want *this
// particular* call to happen", which is the approval handler.
//
// **The approval handler runs before every side-effecting call** -- writing a
// file, fetching a URL -- and returning false refuses it with a message the
// model can read and work around. Reading is not gated by default, since a
// confirmation per read makes an agent unusable and the sandbox already bounds
// what is readable; setApproveReads(true) gates those too.
//
// The tool objects belong to this object, so a caller keeps one DefaultTools
// alive for as long as the registry is used and does not have to track four
// separate lifetimes.
class QTOPENAI_TOOLS_EXPORT DefaultTools : public QObject
{
    Q_OBJECT
public:
    // Return false to refuse the call. Called on the thread that is dispatching
    // the tool, before the tool runs.
    using ApprovalHandler = std::function<bool(const QString &tool, const QJsonObject &arguments)>;

    explicit DefaultTools(QObject *parent = nullptr);
    ~DefaultTools() override;

    // Register everything the policy allows. Returns the names installed, which
    // is what an application should show a user rather than what it asked for:
    // a policy asking for file access with an empty sandbox installs nothing,
    // and the returned list is how the caller finds that out.
    QStringList install(Client::ToolRegistry *registry, const ToolPolicy &policy);

    // Asked before every side-effecting call. Unset means allow.
    void setApprovalHandler(ApprovalHandler handler);
    bool hasApprovalHandler() const;

    // Also ask before reads. Off by default -- a confirmation per read makes an
    // agent unusable, and the sandbox already bounds what can be read.
    void setApproveReads(bool enabled);
    bool approvesReads() const;

    // The tool objects, for connecting to their refused()/performed() signals.
    // Null until install() has created them.
    class FileTools *fileTools() const;
    class HttpTools *httpTools() const;
    class UtilityTools *utilityTools() const;

Q_SIGNALS:
    // Every call the approval handler turned down. An application that logs
    // this can see what a model was talked into trying.
    void denied(const QString &tool, const QJsonObject &arguments);

private:
    Q_DECLARE_PRIVATE(DefaultTools)
    QScopedPointer<DefaultToolsPrivate> d_ptr;
};

} // namespace Tools
} // namespace QtOpenAi
