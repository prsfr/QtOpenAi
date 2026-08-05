// SPDX-License-Identifier: MIT
//
// What did last week cost, and what spent it?
//
// The administration surface answers both from the same query type: a time
// window, a bucket width and a grouping, sent to one of the ten
// /organization/usage reports or to /organization/costs.
//
//   Admin::UsageQuery query;
//   query.startTime = QDateTime::currentSecsSinceEpoch() - 7 * 24 * 3600;
//   query.bucketWidth = QStringLiteral("1d");
//   query.groupBy = {QStringLiteral("model")};
//
//   organization.usage(Admin::Organization::UsageKind::Completions, query);
//   organization.costs(query);
//
// This needs an **admin** API key, not a standard one.
//
// Usage:
//   export OPENAI_ADMIN_KEY=sk-admin-...
//   ./organization_usage            # last 7 days of completions, by model
//   ./organization_usage --costs    # last 7 days of spend, by line item

#include <QtOpenAi/Admin/Organization.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

QString day(qint64 secs)
{
    return QDateTime::fromSecsSinceEpoch(secs).toUTC().toString(QStringLiteral("yyyy-MM-dd"));
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

    const bool wantCosts = app.arguments().contains(QStringLiteral("--costs"));

    Admin::Organization organization(QUrl(baseUrl), adminKey);

    Admin::UsageQuery query;
    // start_time is required by the API — a report with no window is refused.
    query.startTime = QDateTime::currentSecsSinceEpoch() - 7 * 24 * 3600;
    query.bucketWidth = QStringLiteral("1d");
    // Ungrouped, each bucket holds one total row. Grouped, it holds one row per
    // model (or per line item, which is what the costs report splits by).
    query.groupBy = {wantCosts ? QStringLiteral("line_item") : QStringLiteral("model")};

    const auto onError = [&](const Client::ClientError &error) {
        out << "Error: " << error.message() << "\n";
        app.quit();
    };

    if (wantCosts) {
        Admin::CostsReply *reply = organization.costs(query);
        QObject::connect(reply, &Admin::CostsReply::failed, onError);
        QObject::connect(reply, &Admin::CostsReply::finished, [&](const Core::CostPage &costs) {
            double total = 0.0;
            QString currency;
            for (const Core::CostBucket &bucket : costs.data) {
                out << day(bucket.startTime) << "\n";
                for (const Core::CostResult &row : bucket.results) {
                    out << "  " << QString::number(row.amount().value, 'f', 4) << " "
                        << row.amount().currency << "  " << row.lineItem() << "\n";
                    total += row.amount().value;
                    currency = row.amount().currency;
                }
            }
            out << "total: " << QString::number(total, 'f', 2) << " " << currency << "\n";
            app.quit();
        });
    } else {
        Admin::UsageReply *reply
                = organization.usage(Admin::Organization::UsageKind::Completions, query);
        QObject::connect(reply, &Admin::UsageReply::failed, onError);
        QObject::connect(reply, &Admin::UsageReply::finished, [&](const Core::UsagePage &usage) {
            for (const Core::UsageBucket &bucket : usage.data) {
                out << day(bucket.startTime) << "\n";
                // An empty bucket is a day with no traffic, and the server sends
                // it on purpose — keep the gap rather than skipping the line.
                if (bucket.isEmpty())
                    out << "  (nothing)\n";
                for (const Core::UsageResult &row : bucket.results) {
                    out << "  " << row.model() << "  in " << row.inputTokens() << " (cached "
                        << row.inputCachedTokens() << ")  out " << row.outputTokens() << "  over "
                        << row.numModelRequests() << " request(s)\n";
                }
            }
            // next_page is an opaque cursor; pass it back as UsageQuery::page.
            if (usage.hasMore)
                out << "(more: page=" << usage.nextPage << ")\n";
            app.quit();
        });
    }

    return app.exec();
}
