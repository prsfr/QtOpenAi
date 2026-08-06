// SPDX-License-Identifier: MIT
//
// A project and what lives inside it: members, service accounts, API keys and
// per-model rate limits.
//
//   organization.listProjects();
//   organization.listProjectApiKeys(projectId);
//   organization.archiveProject(projectId);      // a POST, not a DELETE
//
// **Archiving is not deleting.** A project is what usage and cost records point
// at, so the API has no way to remove one; archiving sets its status and the
// billing history keeps explaining itself.
//
// This needs an **admin** API key, not a standard one.
//
// Usage:
//   export OPENAI_ADMIN_KEY=sk-admin-...
//   ./organization_projects                  # every active project
//   ./organization_projects proj_abc123      # one project, in detail

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
                    : QStringLiteral("never");
}

QString limit(const std::optional<qint64> &value)
{
    // "unset" and "0" are different answers, and printing them the same way
    // would hide the one that makes the model unusable.
    return value ? QString::number(*value) : QStringLiteral("-");
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

    Admin::Organization organization(QUrl(baseUrl), adminKey);

    const auto onError = [&](const Client::ClientError &error) {
        out << "Error: " << error.message() << "\n";
        app.quit();
    };

    if (projectId.isEmpty()) {
        Admin::ProjectListReply *reply = organization.listProjects();
        QObject::connect(reply, &Admin::ProjectListReply::failed, onError);
        QObject::connect(reply, &Admin::ProjectListReply::finished,
                         [&](const Core::ProjectList &projects) {
                             out << projects.size() << " project(s)\n";
                             for (const Core::Project &project : projects.data) {
                                 out << "  " << project.id() << "  " << project.name() << "  ("
                                     << project.status() << ")\n";
                             }
                             out << "Pass one of these ids to see inside it.\n";
                             app.quit();
                         });
        return app.exec();
    }

    Admin::ProjectApiKeyListReply *keys = organization.listProjectApiKeys(projectId);
    QObject::connect(keys, &Admin::ProjectApiKeyListReply::failed, onError);
    QObject::connect(
            keys, &Admin::ProjectApiKeyListReply::finished,
            [&](const Core::ProjectApiKeyList &page) {
                out << page.size() << " API key(s)\n";
                for (const Core::ProjectApiKey &key : page.data) {
                    // Which kind of principal holds a key is the question an
                    // audit asks first, so the two are not flattened into one.
                    out << "  " << key.redactedValue() << "  " << key.name() << "\n";
                    out << "      owner:     " << key.owner().type() << " " << key.owner().name()
                        << "\n";
                    out << "      last used: " << stamp(key.lastUsedAt()) << "\n";
                }

                Admin::ProjectRateLimitListReply *limits
                        = organization.listProjectRateLimits(projectId);
                QObject::connect(limits, &Admin::ProjectRateLimitListReply::failed, onError);
                QObject::connect(limits, &Admin::ProjectRateLimitListReply::finished,
                                 [&](const Core::ProjectRateLimitList &rates) {
                                     out << rates.size() << " rate limit(s)\n";
                                     for (const Core::ProjectRateLimit &rate : rates.data) {
                                         out << "  " << rate.model() << "  "
                                             << limit(rate.maxRequestsPerMinute()) << " req/min  "
                                             << limit(rate.maxTokensPerMinute()) << " tok/min\n";
                                     }
                                     app.quit();
                                 });
            });

    return app.exec();
}
