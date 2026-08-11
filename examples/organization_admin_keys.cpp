// SPDX-License-Identifier: MIT
//
// The organization's admin API keys -- the credentials that reach the rest of
// the administration surface, including this endpoint.
//
//   organization.listAdminApiKeys();                    // never any secrets
//   organization.createAdminApiKey("Deploy", 2592000);  // the secret, once
//   organization.deleteAdminApiKey(keyId);              // revoke
//
// **The secret is returned exactly once, on creation.** No later list or read
// gives it back -- an endpoint that handed live credentials out on demand would
// turn one admin key into a master key. So `value()` is filled in only on the
// key that comes straight from a create, and an empty `value()` on a listed key
// is the API saying so, not a decode that failed. Store it at once or lose it.
//
// **Do not print it.** This example deliberately does not, and neither does the
// library: Client::LoggingInterceptor redacts `value` out of any body it logs.
// `redactedValue()` -- `sk-admin...def` -- is the one meant to be shown.
//
// This needs an **admin** API key, not a standard one.
//
// Usage:
//   export OPENAI_ADMIN_KEY=sk-admin-...
//   ./organization_admin_keys                    # list every admin key
//   ./organization_admin_keys "Deploy key"       # create one (prints nothing secret)
//   ./organization_admin_keys "Deploy key" 2592000   # ...expiring in 30 days

#include <QtOpenAi/Admin/Organization.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

QString stamp(qint64 secs)
{
    // 0 is "no such moment" -- a key that never expires, or has never been used
    // -- rather than 1970.
    return secs > 0 ? QDateTime::fromSecsSinceEpoch(secs).toString(Qt::ISODate)
                    : QStringLiteral("never");
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

    const QString name = app.arguments().value(1);
    const int expiresInSeconds = app.arguments().value(2).toInt();

    Admin::Organization organization(QUrl(baseUrl), adminKey);

    const auto onError = [&](const Client::ClientError &error) {
        out << "Error: " << error.message() << "\n";
        app.quit();
    };

    if (name.isEmpty()) {
        Admin::AdminApiKeyListReply *reply = organization.listAdminApiKeys();
        QObject::connect(reply, &Admin::AdminApiKeyListReply::failed, onError);
        QObject::connect(reply, &Admin::AdminApiKeyListReply::finished,
                         [&](const Core::AdminApiKeyList &page) {
                             out << page.size() << " admin key(s)\n";
                             for (const Core::AdminApiKey &key : page.data) {
                                 out << "  " << key.id() << "  "
                                     << (key.name().isEmpty() ? QStringLiteral("(unnamed)")
                                                              : key.name())
                                     << "  " << key.redactedValue() << "\n";
                                 out << "      created:   " << stamp(key.createdAt()) << "\n";
                                 out << "      expires:   " << stamp(key.expiresAt()) << "\n";
                                 // A key nobody has ever used is the one worth
                                 // asking about at the next review.
                                 out << "      last used: " << stamp(key.lastUsedAt()) << "\n";
                                 out << "      owner:     " << key.owner().name() << "\n";
                                 // Always empty here, and that is the API being
                                 // careful rather than this example being lazy.
                                 out << "      secret:    "
                                     << (key.hasValue() ? QStringLiteral("present")
                                                        : QStringLiteral("not returned on a read"))
                                     << "\n";
                             }
                             out << "Pass a name to create a key.\n";
                             app.quit();
                         });
        return app.exec();
    }

    // 0 seconds means a key that does not expire; the argument is omitted from
    // the request rather than sent as a zero.
    Admin::AdminApiKeyReply *reply = organization.createAdminApiKey(name, expiresInSeconds);
    QObject::connect(reply, &Admin::AdminApiKeyReply::failed, onError);
    QObject::connect(reply, &Admin::AdminApiKeyReply::finished, [&](const Core::AdminApiKey &key) {
        out << "Created " << key.id() << "  " << key.name() << "\n";
        out << "  expires: " << stamp(key.expiresAt()) << "\n";
        out << "  shown as: " << key.redactedValue() << "\n";

        // The one moment the secret exists outside the server.
        // It is *not* printed here: a terminal buffer is a log
        // like any other. A real caller writes it straight to
        // wherever it belongs -- a secret manager, a CI
        // variable -- and never to stdout.
        if (key.hasValue()) {
            out << "  secret:   returned, " << key.value().size()
                << " characters -- store it now, it cannot be fetched again\n";
            // e.g. secretManager.store(key.value());
        } else {
            out << "  secret:   absent, which should not happen on a create\n";
        }
        app.quit();
    });

    return app.exec();
}
