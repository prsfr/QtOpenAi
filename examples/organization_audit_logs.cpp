// SPDX-License-Identifier: MIT
//
// The organization's audit trail: what happened, who did it, and to what.
//
//   Admin::AuditLogQuery query;
//   query.effectiveAtGte = since;
//   query.eventTypes = {QStringLiteral("project.archived")};
//   organization.listAuditLogs(query);
//
// **There is no event-type enum, and that is the design.** The API describes
// ~55 event types and adds more without notice, and it does not nest the payload
// under a `data` field -- it puts it under a key *named after the type*:
//
//   {"type": "project.archived", "project.archived": {"id": "proj_abc"}}
//
// So `payload()` is found by looking up `type()`, and an event type this build
// has never heard of reads exactly as well as one it was written against. This
// example prints unknown types alongside known ones without doing anything
// special for them, which is the point.
//
// **A filter the server does not recognise is ignored, not refused** -- so a
// wrong query comes back as a valid page of the wrong events. See
// Admin::AuditLogQuery.
//
// This needs an **admin** API key, not a standard one.
//
// Usage:
//   export OPENAI_ADMIN_KEY=sk-admin-...
//   ./organization_audit_logs              # the last 24 hours
//   ./organization_audit_logs 168          # the last week, in hours
//   ./organization_audit_logs 24 project.archived   # ...one event type only

#include <QtOpenAi/Admin/Organization.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QJsonDocument>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

QString stamp(qint64 secs)
{
    return secs > 0 ? QDateTime::fromSecsSinceEpoch(secs).toString(Qt::ISODate)
                    : QStringLiteral("-");
}

// Who did it, in one line. Both halves of the actor union answer the same
// question, so this asks it once rather than branching on the kind first.
QString who(const Core::AuditLogActor &actor)
{
    if (!actor.user().email().isEmpty())
        return actor.user().email();
    if (actor.isServiceAccount())
        return QStringLiteral("service account %1").arg(actor.serviceAccountId());
    if (!actor.apiKeyId().isEmpty())
        return QStringLiteral("api key %1").arg(actor.apiKeyId());
    return QStringLiteral("(unattributed)");
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

    const int hours = app.arguments().value(1).toInt() > 0 ? app.arguments().value(1).toInt() : 24;
    const QString eventType = app.arguments().value(2);

    Admin::AuditLogQuery query;
    // gte rather than gt: the boundary second belongs in the window. With
    // several events in the same second that is a real difference.
    query.effectiveAtGte = QDateTime::currentSecsSinceEpoch() - qint64(hours) * 3600;
    if (!eventType.isEmpty())
        query.eventTypes = {eventType};
    query.limit = 20;

    Admin::Organization organization(QUrl(baseUrl), adminKey);

    Admin::AuditLogListReply *reply = organization.listAuditLogs(query);
    QObject::connect(reply, &Admin::AuditLogListReply::failed,
                     [&](const Client::ClientError &error) {
                         out << "Error: " << error.message() << "\n";
                         app.quit();
                     });
    QObject::connect(
            reply, &Admin::AuditLogListReply::finished, [&](const Core::AuditLogList &page) {
                out << page.size() << " event(s) in the last " << hours << " hour(s)\n";
                for (const Core::AuditLog &entry : page.data) {
                    out << "  " << stamp(entry.effectiveAt()) << "  " << entry.type() << "\n";
                    out << "      by:       " << who(entry.actor());
                    if (!entry.actor().ipAddress().isEmpty())
                        out << "  from " << entry.actor().ipAddress();
                    out << "\n";
                    if (!entry.resourceId().isEmpty())
                        out << "      subject:  " << entry.resourceId() << "\n";
                    if (!entry.projectId().isEmpty())
                        out << "      project:  " << entry.projectName() << " ("
                            << entry.projectId() << ")\n";

                    // `data` is what a creation was given; `changes_requested`
                    // is what an update asked to change. Different claims, so
                    // they are printed under different labels.
                    if (!entry.data().isEmpty())
                        out << "      data:     "
                            << QString::fromUtf8(
                                       QJsonDocument(entry.data()).toJson(QJsonDocument::Compact))
                            << "\n";
                    if (!entry.changesRequested().isEmpty())
                        out << "      wanted:   "
                            << QString::fromUtf8(QJsonDocument(entry.changesRequested())
                                                         .toJson(QJsonDocument::Compact))
                            << "\n";
                }
                if (page.hasMore)
                    out << "More available; pass last_id as AuditLogQuery::after: " << page.lastId
                        << "\n";
                app.quit();
            });

    return app.exec();
}
