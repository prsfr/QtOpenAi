// SPDX-License-Identifier: MIT
//
// Who is in the organization, and who has been invited?
//
// Membership has two halves. An Invite is the pending one: it exists until it is
// accepted, expires, or is withdrawn, and only then does an OrganizationUser
// appear. There is no endpoint that creates a member directly.
//
//   organization.listUsers();                       // who is in
//   organization.listInvites();                     // who has been asked
//   organization.createInvite(request);             // ask someone
//   organization.modifyUserRole(id, "owner");       // promote a member
//   organization.deleteUser(id);                    // remove one
//
// This needs an **admin** API key, not a standard one. Note what that key can
// do here: the last two lines change who has access to the organization.
//
// Usage:
//   export OPENAI_ADMIN_KEY=sk-admin-...
//   ./organization_members                     # members and open invitations
//   ./organization_members --invite a@b.com    # invite a reader

#include <QtOpenAi/Admin/Organization.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

QString stamp(qint64 secs)
{
    return secs > 0 ? QDateTime::fromSecsSinceEpoch(secs).toString(Qt::ISODate)
                    : QStringLiteral("-");
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

    const QStringList arguments = app.arguments();
    const int inviteAt = arguments.indexOf(QStringLiteral("--invite"));
    const QString inviteEmail = inviteAt >= 0 ? arguments.value(inviteAt + 1) : QString();

    Admin::Organization organization(QUrl(baseUrl), adminKey);

    const auto onError = [&](const Client::ClientError &error) {
        out << "Error: " << error.message() << "\n";
        app.quit();
    };

    if (!inviteEmail.isEmpty()) {
        // "reader" rather than "owner": an example that hands out the ability to
        // remove people would be a poor default to copy.
        Core::CreateInviteRequest request(inviteEmail, QStringLiteral("reader"));

        Admin::InviteReply *reply = organization.createInvite(request);
        QObject::connect(reply, &Admin::InviteReply::failed, onError);
        QObject::connect(reply, &Admin::InviteReply::finished, [&](const Core::Invite &invite) {
            out << "invited " << invite.email() << " as " << invite.role() << "\n";
            out << "  id:      " << invite.id() << "\n";
            out << "  status:  " << invite.status() << "\n";
            out << "  expires: " << stamp(invite.expiresAt()) << "\n";
            app.quit();
        });
        return app.exec();
    }

    Admin::UserListReply *users = organization.listUsers();
    QObject::connect(users, &Admin::UserListReply::failed, onError);
    QObject::connect(
            users, &Admin::UserListReply::finished, [&](const Core::OrganizationUserList &page) {
                out << page.size() << " member(s)\n";
                for (const Core::OrganizationUser &user : page.data) {
                    out << "  " << user.email() << "  " << user.role() << "  since "
                        << stamp(user.addedAt()) << "\n";
                }
                if (page.hasMore)
                    out << "(more pages: after=" << page.lastId << ")\n";

                // Chained rather than issued in parallel, so the two lists do not
                // interleave in the output.
                Admin::InviteListReply *invites = organization.listInvites();
                QObject::connect(invites, &Admin::InviteListReply::failed, onError);
                QObject::connect(
                        invites, &Admin::InviteListReply::finished,
                        [&](const Core::InviteList &open) {
                            out << open.size() << " invitation(s)\n";
                            for (const Core::Invite &invite : open.data) {
                                out << "  " << invite.email() << "  " << invite.role() << "  "
                                    << invite.status() << "  expires " << stamp(invite.expiresAt())
                                    << "\n";
                                for (const Core::InviteProject &project : invite.projects())
                                    out << "      " << project.id << ": " << project.role << "\n";
                            }
                            app.quit();
                        });
            });

    return app.exec();
}
