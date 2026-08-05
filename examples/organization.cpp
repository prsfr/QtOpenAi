// SPDX-License-Identifier: MIT
//
// The administration surface: list the organization's projects.
//
// This one needs an **admin** API key, not a standard one, and it is a separate
// object for exactly that reason:
//
//   Admin::Organization organization(baseUrl, adminKey);
//   organization.listProjects();
//
// An admin key can archive a project or revoke a colleague's access. Hanging
// these endpoints off Client::Client would have put a credential of that reach
// on the same object an application uses to answer a user's question -- and
// nothing would then stop it from being sent to /chat/completions. Two objects,
// two keys.
//
// It is not a second networking stack, though: requests run through Client's
// documented request path, so the retry policy, the interceptor chain (with the
// credential redacted in the log) and the rate limiter all apply here too.
//
// Usage:
//   export OPENAI_ADMIN_KEY=sk-admin-...
//   ./organization              # active projects
//   ./organization --archived   # archived ones as well

#include <QtOpenAi/Admin/Organization.h>
#include <QtOpenAi/Client/LoggingInterceptor.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
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

    const bool includeArchived = app.arguments().contains(QStringLiteral("--archived"));

    Admin::Organization organization(QUrl(baseUrl), adminKey);

    // Worth having here in particular: an admin key in a log is a worse
    // accident than a standard one, and this interceptor redacts credentials.
    Client::LoggingInterceptor logger;
    organization.addInterceptor(&logger);

    Admin::ProjectListReply *reply = organization.listProjects({}, includeArchived);

    QObject::connect(reply, &Admin::ProjectListReply::failed,
                     [&](const Client::ClientError &error) {
                         out << "Error: " << error.message() << "\n";
                         app.quit();
                     });

    QObject::connect(
            reply, &Admin::ProjectListReply::finished, [&](const Core::ProjectList &projects) {
                out << projects.size() << " project(s)\n";
                for (const Core::Project &project : projects.data) {
                    out << "  " << project.id() << "  " << project.name() << "\n";
                    out << "    status:  " << project.status() << "\n";
                    out << "    created: "
                        << QDateTime::fromSecsSinceEpoch(project.createdAt()).toString(Qt::ISODate)
                        << "\n";
                    if (project.isArchived()) {
                        out << "    archived:"
                            << QDateTime::fromSecsSinceEpoch(project.archivedAt())
                                        .toString(Qt::ISODate)
                            << "\n";
                    }
                }
                // has_more means there is another page; pass the last
                // id back as ListParams::after to walk it.
                if (projects.hasMore)
                    out << "(more pages: after=" << projects.lastId << ")\n";
                app.quit();
            });

    return app.exec();
}
