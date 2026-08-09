// SPDX-License-Identifier: MIT
//
// What a project is allowed to run: which models, and which hosted tools.
//
//   organization.getProjectModelPermissions(projectId);        // a policy
//   organization.getProjectHostedToolPermissions(projectId);   // five switches
//
// **The model policy is not a grant list.** The same model id means "permitted"
// under an allow list and "forbidden" under a deny list, so the ids alone do not
// answer the only question worth asking. Ask `allowsModel()` instead -- and note
// it answers `std::nullopt` for a mode this build cannot read, rather than
// guessing "allowed" and failing open on a deny list.
//
// **The two surfaces are different shapes**, which is why they are different
// types: one is a mode plus a list, the other is a fixed record of named
// on/off switches with no mode at all.
//
// **A hosted-tool update is partial.** Only the tools you set are sent, so the
// ones you never mention are left alone rather than switched off.
//
// This needs an **admin** API key, not a standard one.
//
// Usage:
//   export OPENAI_ADMIN_KEY=sk-admin-...
//   ./organization_permissions proj_abc123            # read both surfaces
//   ./organization_permissions proj_abc123 gpt-4.1    # ...and ask about a model

#include <QtOpenAi/Admin/Organization.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

QString switchState(const std::optional<bool> &enabled)
{
    // "unset" and "off" are different answers: a tool the record never mentions
    // is one this build has no reading for, not one that is switched off.
    if (!enabled)
        return QStringLiteral("unset");
    return *enabled ? QStringLiteral("on") : QStringLiteral("off");
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString adminKey = env.value(QStringLiteral("OPENAI_ADMIN_KEY"));
    const QString baseUrl = env.value(QStringLiteral("OPENAI_BASE_URL"),
                                      QStringLiteral("https://api.openai.com/v1"));
    if (adminKey.isEmpty()) {
        out << "Set OPENAI_ADMIN_KEY to run this example.\n";
        out << "It must be an admin key; a standard API key cannot read this surface.\n";
        return 1;
    }

    const QString projectId = app.arguments().value(1);
    if (projectId.isEmpty()) {
        out << "Usage: organization_permissions <project-id> [model-id]\n";
        out << "Run the organization_projects example to list project ids.\n";
        return 1;
    }
    const QString modelId = app.arguments().value(2);

    Admin::Organization organization(QUrl(baseUrl), adminKey);

    const auto onError = [&](const Client::ClientError &error) {
        out << "Error: " << error.message() << "\n";
        app.quit();
    };

    // Second half of the run: the hosted-tool switches. Read after the model
    // policy so the two arrive in a fixed order.
    const auto readHostedTools = [&] {
        Admin::ProjectHostedToolPermissionsReply *reply =
                organization.getProjectHostedToolPermissions(projectId);
        QObject::connect(reply, &Admin::ProjectHostedToolPermissionsReply::failed, onError);
        QObject::connect(reply, &Admin::ProjectHostedToolPermissionsReply::finished,
                         [&](const Core::ProjectHostedToolPermissions &tools) {
                             out << "\nHosted tools\n";
                             out << "  file_search:      " << switchState(tools.fileSearch())
                                 << "\n";
                             out << "  web_search:       " << switchState(tools.webSearch())
                                 << "\n";
                             out << "  image_generation: " << switchState(tools.imageGeneration())
                                 << "\n";
                             out << "  mcp:              " << switchState(tools.mcp()) << "\n";
                             out << "  code_interpreter: " << switchState(tools.codeInterpreter())
                                 << "\n";

                             // A tool added to the API after this build has no
                             // named accessor above, and is still carried.
                             const QMap<QString, bool> all = tools.permissions();
                             for (auto it = all.constBegin(); it != all.constEnd(); ++it) {
                                 if (!tools.isKnownTool(it.key()))
                                     out << "  " << it.key() << ": "
                                         << (it.value() ? "on" : "off") << "  (not named by "
                                         << "this build)\n";
                             }

                             // Switching one tool off would send exactly that one
                             // tool, leaving the other four untouched:
                             //
                             //   Core::ProjectHostedToolPermissions update;
                             //   update.setWebSearch(false);
                             //   organization.setProjectHostedToolPermissions(projectId, update);
                             app.quit();
                         });
    };

    Admin::ProjectModelPermissionsReply *reply = organization.getProjectModelPermissions(projectId);
    QObject::connect(reply, &Admin::ProjectModelPermissionsReply::failed, onError);
    QObject::connect(reply, &Admin::ProjectModelPermissionsReply::finished,
                     [&](const Core::ProjectModelPermissions &policy) {
                         out << "Model policy for " << projectId << "\n";
                         out << "  mode: "
                             << (policy.mode().isEmpty() ? QStringLiteral("(none -- the project "
                                                                          "inherits the org's)")
                                                         : policy.mode())
                             << "\n";
                         for (const QString &id : policy.modelIds())
                             out << "  - " << id << "\n";

                         if (!modelId.isEmpty()) {
                             // The point of the whole class: this is the answer,
                             // and it is not "is the id in the list".
                             const std::optional<bool> allowed = policy.allowsModel(modelId);
                             out << "  " << modelId << ": ";
                             if (!allowed)
                                 out << "unknown -- this build cannot read mode \"" << policy.mode()
                                     << "\", and guessing would fail open\n";
                             else
                                 out << (*allowed ? "allowed" : "forbidden") << "\n";
                         }

                         readHostedTools();
                     });

    return app.exec();
}
