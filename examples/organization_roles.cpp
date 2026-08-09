// SPDX-License-Identifier: MIT
//
// Who can do what: the organization's roles, its groups, and the roles a group
// holds.
//
//   organization.listRoles();                                  // the organization's
//   organization.listRoles(Admin::RoleScope::project(id));     // one project's
//   organization.listGroupRoles(groupId);
//
// **Scope is an argument, not a second set of methods.** An organization role
// and a project role are the same payload with a different `resource_type`, so
// one method serves both -- and RoleScope is what knows that a project's roles
// live under /projects/{id}/roles rather than under /organization/projects.
//
// **A role can be inherited.** A role a group gave a user is listed for the user
// with the group named in `assignmentSources()`; revoking it from the user does
// nothing, because the user never had it directly.
//
// This needs an **admin** API key, not a standard one.
//
// Usage:
//   export OPENAI_ADMIN_KEY=sk-admin-...
//   ./organization_roles                     # roles and groups
//   ./organization_roles group_01J1F8ABC     # one group's roles and members

#include <QtOpenAi/Admin/Organization.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

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

    const QString groupId = app.arguments().value(1);

    Admin::Organization organization(QUrl(baseUrl), adminKey);

    const auto onError = [&](const Client::ClientError &error) {
        out << "Error: " << error.message() << "\n";
        app.quit();
    };

    if (groupId.isEmpty()) {
        Admin::RoleListReply *roles = organization.listRoles();
        QObject::connect(roles, &Admin::RoleListReply::failed, onError);
        QObject::connect(
                roles, &Admin::RoleListReply::finished,
                [&](const Core::OrganizationRoleList &page) {
                    out << page.size() << " role(s)\n";
                    for (const Core::OrganizationRole &role : page.data) {
                        // A predefined role is OpenAI's and cannot be edited or
                        // deleted, which is worth knowing before offering to.
                        out << "  " << role.id() << "  " << role.name()
                            << (role.predefinedRole() ? "  [predefined]" : "") << "\n";
                        out << "      " << role.permissions().join(QStringLiteral(", ")) << "\n";
                    }

                    Admin::GroupListReply *groups = organization.listGroups();
                    QObject::connect(groups, &Admin::GroupListReply::failed, onError);
                    QObject::connect(groups, &Admin::GroupListReply::finished,
                                     [&](const Core::GroupList &page) {
                                         out << page.size() << " group(s)\n";
                                         for (const Core::Group &group : page.data) {
                                             // A synchronised group's membership
                                             // belongs to the identity provider:
                                             // a change here is undone by the
                                             // next sync.
                                             out << "  " << group.id() << "  " << group.name()
                                                 << (group.isScimManaged() ? "  [SCIM]" : "")
                                                 << "\n";
                                         }
                                         out << "Pass one of these ids to see inside it.\n";
                                         app.quit();
                                     });
                });
        return app.exec();
    }

    Admin::RoleListReply *assigned = organization.listGroupRoles(groupId);
    QObject::connect(assigned, &Admin::RoleListReply::failed, onError);
    QObject::connect(
            assigned, &Admin::RoleListReply::finished, [&](const Core::OrganizationRoleList &page) {
                out << page.size() << " role(s) held by " << groupId << "\n";
                for (const Core::OrganizationRole &role : page.data)
                    out << "  " << role.name() << "  (" << role.resourceType() << ")\n";

                Admin::GroupMemberListReply *members = organization.listGroupUsers(groupId);
                QObject::connect(members, &Admin::GroupMemberListReply::failed, onError);
                QObject::connect(members, &Admin::GroupMemberListReply::finished,
                                 [&](const Core::GroupMemberList &page) {
                                     out << page.size() << " member(s)\n";
                                     for (const Core::GroupMember &member : page.data) {
                                         out << "  " << member.name() << "  " << member.email()
                                             << "\n";
                                     }
                                     app.quit();
                                 });
            });

    return app.exec();
}
