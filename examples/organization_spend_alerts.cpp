// SPDX-License-Identifier: MIT
//
// Spend alerts and data-retention settings, at the organization and at a
// project.
//
//   organization.listSpendAlerts();
//   organization.createSpendAlert(alert);
//   organization.getDataRetention();
//   organization.setProjectDataRetention(projectId, "organization_default");
//
// **The threshold is in cents.** 100000 is $1,000.00. This example prints both
// numbers so the factor is visible; getting it wrong by a hundred means an alert
// that fires on the first dollar of the month, or one that never fires at all
// and is not missed until the invoice arrives.
//
// **Reading and writing retention use different field names** -- the resource
// reports `type`, the update body takes `retention_type` -- so the setter takes
// the value directly rather than a Core::DataRetention. See that class.
//
// This needs an **admin** API key, not a standard one.
//
// Usage:
//   export OPENAI_ADMIN_KEY=sk-admin-...
//   ./organization_spend_alerts                 # alerts + org retention
//   ./organization_spend_alerts proj_abc123     # ...and that project's

#include <QtOpenAi/Admin/Organization.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

// Cents as the API counts them, and money as a person reads it. Kept here in
// the presentation layer rather than in the value type, so that there is one
// place the factor is applied and it is a visible one.
QString money(qint64 cents)
{
    return QStringLiteral("%1.%2 (%3 cents)")
            .arg(cents / 100)
            .arg(qAbs(cents % 100), 2, 10, QLatin1Char('0'))
            .arg(cents);
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

    // Third and last step: a project's retention setting, when one was named.
    const auto readProjectRetention = [&] {
        if (projectId.isEmpty()) {
            out << "\nPass a project id to see that project's retention setting.\n";
            app.quit();
            return;
        }
        Admin::DataRetentionReply *reply = organization.getProjectDataRetention(projectId);
        QObject::connect(reply, &Admin::DataRetentionReply::failed, onError);
        QObject::connect(reply, &Admin::DataRetentionReply::finished,
                         [&](const Core::DataRetention &retention) {
                             out << "\nProject " << projectId << " retention: " << retention.type();
                             // Two of the six values belong to the project scope
                             // alone, and this is the one worth naming: it is
                             // not a policy, it is a deferral.
                             if (retention.isOrganizationDefault())
                                 out << "  (inherits the organization's)";
                             out << "\n";
                             app.quit();
                         });
    };

    // Second step: the organization's retention setting.
    const auto readRetention = [&] {
        Admin::DataRetentionReply *reply = organization.getDataRetention();
        QObject::connect(reply, &Admin::DataRetentionReply::failed, onError);
        QObject::connect(reply, &Admin::DataRetentionReply::finished,
                         [&](const Core::DataRetention &retention) {
                             out << "\nOrganization retention: " << retention.type() << "\n";
                             // Changing it would be:
                             //
                             //   organization.setDataRetention(
                             //           QStringLiteral("zero_data_retention"));
                             //
                             // ...which sends {"retention_type": ...}, not
                             // {"type": ...}. Not done here: it is a compliance
                             // setting, not something an example should alter.
                             readProjectRetention();
                         });
    };

    Admin::SpendAlertListReply *reply = organization.listSpendAlerts();
    QObject::connect(reply, &Admin::SpendAlertListReply::failed, onError);
    QObject::connect(
            reply, &Admin::SpendAlertListReply::finished, [&](const Core::SpendAlertList &page) {
                out << page.size() << " spend alert(s)\n";
                for (const Core::SpendAlert &alert : page.data) {
                    out << "  " << alert.id() << "  over " << money(alert.thresholdAmount()) << " "
                        << alert.currency() << " per " << alert.interval() << "\n";
                    const Core::SpendAlertNotificationChannel channel = alert.notificationChannel();
                    out << "      " << channel.type() << " to "
                        << channel.recipients().join(QStringLiteral(", "));
                    if (!channel.subjectPrefix().isEmpty())
                        out << "  subject prefix: " << channel.subjectPrefix();
                    out << "\n";
                }

                // Creating one would be:
                //
                //   Core::SpendAlertNotificationChannel channel;
                //   channel.setRecipients({QStringLiteral("finance@example.com")});
                //   Core::SpendAlert alert;
                //   alert.setThresholdAmount(100000);   // $1,000.00
                //   alert.setNotificationChannel(channel);
                //   organization.createSpendAlert(alert);
                //
                // currency and interval default to the only values the
                // API accepts, so they need not be set.
                readRetention();
            });

    return app.exec();
}
