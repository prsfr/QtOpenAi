// SPDX-License-Identifier: MIT
//
// The organization's client certificates, and which of them a scope has
// switched on.
//
//   organization.listCertificates();                       // uploaded to the org
//   organization.listProjectCertificates(projectId);       // active in a project
//   organization.activateCertificates({certificateId});    // a batch, always
//
// **Activation takes a batch, not a certificate.** `activate` and `deactivate`
// are POSTs to a path ending in the verb, carrying a list of ids -- there is no
// per-certificate endpoint, however much the method names suggest one.
//
// **The PEM body is asked for, never assumed.** Listings never carry it; a
// single read returns it only when told to, which is what the `true` below is.
//
// This needs an **admin** API key, not a standard one.
//
// Usage:
//   export OPENAI_ADMIN_KEY=sk-admin-...
//   ./organization_certificates                 # every certificate in the org
//   ./organization_certificates cert_abc123     # one certificate, with its PEM

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

QString activation(const std::optional<bool> &active)
{
    // "unset" and "false" are different answers: a certificate read by id
    // belongs to no scope, so the question does not apply to it at all.
    if (!active)
        return QStringLiteral("n/a");
    return *active ? QStringLiteral("active") : QStringLiteral("inactive");
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

    const QString certificateId = app.arguments().value(1);

    Admin::Organization organization(QUrl(baseUrl), adminKey);

    const auto onError = [&](const Client::ClientError &error) {
        out << "Error: " << error.message() << "\n";
        app.quit();
    };

    if (certificateId.isEmpty()) {
        Admin::CertificateListReply *reply = organization.listCertificates();
        QObject::connect(reply, &Admin::CertificateListReply::failed, onError);
        QObject::connect(reply, &Admin::CertificateListReply::finished,
                         [&](const Core::CertificateList &page) {
                             out << page.size() << " certificate(s)\n";
                             for (const Core::Certificate &certificate : page.data) {
                                 out << "  " << certificate.id() << "  "
                                     << (certificate.name().isEmpty() ? QStringLiteral("(unnamed)")
                                                                      : certificate.name())
                                     << "  [" << activation(certificate.active()) << "]\n";
                                 out << "      expires: " << stamp(certificate.expiresAt()) << "\n";
                             }
                             out << "Pass one of these ids to see its PEM content.\n";
                             app.quit();
                         });
        return app.exec();
    }

    // The `true` is what fetches the PEM; without it the reply carries only the
    // validity window.
    Admin::CertificateReply *reply = organization.getCertificate(certificateId, true);
    QObject::connect(reply, &Admin::CertificateReply::failed, onError);
    QObject::connect(reply, &Admin::CertificateReply::finished,
                     [&](const Core::Certificate &certificate) {
                         out << certificate.id() << "  " << certificate.name() << "\n";
                         out << "  uploaded: " << stamp(certificate.createdAt()) << "\n";
                         out << "  valid:    " << stamp(certificate.validAt()) << " .. "
                             << stamp(certificate.expiresAt()) << "\n";
                         // Unset here whatever the certificate's state: this read
                         // belongs to no scope, so there is nothing to be active in.
                         out << "  active:   " << activation(certificate.active()) << "\n";
                         out << certificate.pemContent() << "\n";
                         app.quit();
                     });

    return app.exec();
}
