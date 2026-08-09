// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {

class CertificateData;

// A client certificate uploaded to the organization (GET/POST
// /organization/certificates, GET/POST/DELETE
// /organization/certificates/{certificate_id}, and the listings and
// activation toggles at organization and project scope).
//
// Part of the administration surface, which uses an admin API key rather than a
// standard one — see QtOpenAi::Admin::Organization.
//
// **One class for all three of the API's certificate schemas.** The document
// defines `Certificate`, `OrganizationCertificate` and
// `OrganizationProjectCertificate` separately, but they differ in exactly two
// things: which value `object` carries, and whether the PEM `content` and the
// `active` flag are present. Those are values, not types — the same call
// Core::OrganizationRole makes for its two scopes. Three classes would have been
// two copies that differ by a comment, and a caller listing certificates for a
// screen would need all three.
//
// **The validity window is flattened out of `certificate_details`.** On the wire
// those three fields sit in a nested object; here they are validAt(),
// expiresAt() and pemContent() directly. The nesting groups nothing that means
// anything on its own — it is just what was parsed out of the PEM — and asking
// "when does this expire" should not require walking a sub-object. toJson()
// puts them back where the API expects them.
class QTOPENAI_CORE_EXPORT Certificate
{
public:
    Certificate();
    Certificate(const Certificate &other);
    Certificate(Certificate &&other) noexcept;
    Certificate &operator=(const Certificate &other);
    Certificate &operator=(Certificate &&other) noexcept;
    ~Certificate();

    void swap(Certificate &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // "certificate" from a single read or an upload, "organization.certificate"
    // from the organization listing and its toggles, and
    // "organization.project.certificate" from a project's — or
    // "certificate.deleted" from a deletion. Which scope a certificate was read
    // at is this string, not a separate type.
    QString object() const;
    void setObject(const QString &object);

    bool isProjectScoped() const
    {
        return object() == QLatin1String("organization.project.certificate");
    }

    // Optional on upload, and the API may send null.
    QString name() const;
    void setName(const QString &name);

    // Unix timestamp of the upload; 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    // --- The validity window, from `certificate_details` -------------------
    // Unix timestamps parsed out of the certificate itself; 0 when absent.
    qint64 validAt() const;
    void setValidAt(qint64 validAt);

    qint64 expiresAt() const;
    void setExpiresAt(qint64 expiresAt);

    // **Only present when it was asked for.** The PEM body comes back from
    // uploading, and from a single read that passed `include=content` — see
    // Admin::Organization::getCertificate(). Neither listing returns it, which
    // is the right default: a page of certificates should not carry a page of
    // PEM bodies.
    QString pemContent() const;
    void setPemContent(const QString &pemContent);

    // --- Activation --------------------------------------------------------
    // Whether the certificate is active *at the scope it was read from*, and
    // **unset when that question does not apply**: a single read by id belongs
    // to no scope, so the API sends no `active` there at all.
    //
    // An optional rather than a plain bool, for the reason ProjectRateLimit's
    // limits are optionals: `false` here is a real answer — uploaded but not in
    // force — and defaulting the absent case to it would report every
    // certificate fetched by id as switched off.
    std::optional<bool> active() const;
    void setActive(bool active);

    // True in the answer to DELETE /organization/certificates/{id}, which keeps
    // the id and says so in `object` alone.
    //
    // Derived from the object name rather than stored, unlike the same question
    // on Core::OrganizationRole and Core::Group: those acknowledgements carry a
    // `deleted` boolean and this one does not, so a field here would be one this
    // API never fills in.
    bool isDeleted() const { return object() == QLatin1String("certificate.deleted"); }

    QJsonObject toJson() const;
    static Certificate fromJson(const QJsonObject &json);

    bool operator==(const Certificate &other) const;
    bool operator!=(const Certificate &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CertificateData> d;
};

// A page of certificates, at either scope — and also the answer to activating
// or deactivating a batch, which reports the certificates it changed. Those
// replies carry `data` and no cursors, which reads back as a page whose
// `hasMore` is false: true, and the only honest thing to say about a reply that
// is not paginated at all.
using CertificateList = ListPage<Certificate>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Certificate)
Q_DECLARE_METATYPE(QtOpenAi::Core::Certificate)
Q_DECLARE_METATYPE(QtOpenAi::Core::CertificateList)
