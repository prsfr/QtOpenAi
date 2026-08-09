// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Certificate.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

namespace {
// The sub-object the validity window arrives in. Spelled once so the two halves
// of the round trip cannot drift apart.
constexpr QLatin1String kDetails("certificate_details");
} // namespace

class CertificateData : public QSharedData
{
public:
    QString id;
    QString object;
    QString name;
    qint64 createdAt = 0;
    qint64 validAt = 0;
    qint64 expiresAt = 0;
    QString pemContent;
    std::optional<bool> active;
};

Certificate::Certificate()
    : d(new CertificateData)
{ }

Certificate::Certificate(const Certificate &other) = default;
Certificate::Certificate(Certificate &&other) noexcept = default;
Certificate &Certificate::operator=(const Certificate &other) = default;
Certificate &Certificate::operator=(Certificate &&other) noexcept = default;
Certificate::~Certificate() = default;

QString Certificate::id() const { return d->id; }
void Certificate::setId(const QString &id) { d->id = id; }

QString Certificate::object() const { return d->object; }
void Certificate::setObject(const QString &object) { d->object = object; }

QString Certificate::name() const { return d->name; }
void Certificate::setName(const QString &name) { d->name = name; }

qint64 Certificate::createdAt() const { return d->createdAt; }
void Certificate::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

qint64 Certificate::validAt() const { return d->validAt; }
void Certificate::setValidAt(qint64 validAt) { d->validAt = validAt; }

qint64 Certificate::expiresAt() const { return d->expiresAt; }
void Certificate::setExpiresAt(qint64 expiresAt) { d->expiresAt = expiresAt; }

QString Certificate::pemContent() const { return d->pemContent; }
void Certificate::setPemContent(const QString &pemContent) { d->pemContent = pemContent; }

std::optional<bool> Certificate::active() const { return d->active; }
void Certificate::setActive(bool active) { d->active = active; }

QJsonObject Certificate::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);

    // Nested back where the API keeps them, and left out entirely when there is
    // nothing to nest: an empty `certificate_details` would claim a window this
    // certificate never reported.
    QJsonObject details;
    detail::insertIfNonZero(details, QStringLiteral("valid_at"), d->validAt);
    detail::insertIfNonZero(details, QStringLiteral("expires_at"), d->expiresAt);
    detail::insertIfNotEmpty(details, QStringLiteral("content"), d->pemContent);
    if (!details.isEmpty())
        json.insert(kDetails, details);

    // Written whenever it is set, `false` included -- unlike the flags on the
    // other administration types, an inactive certificate is a state the server
    // really reports and not the absence of one.
    detail::insertIfSet(json, QStringLiteral("active"), d->active);
    return json;
}

Certificate Certificate::fromJson(const QJsonObject &json)
{
    Certificate certificate;
    certificate.d->id = detail::stringOr(json, QStringLiteral("id"));
    certificate.d->object = detail::stringOr(json, QStringLiteral("object"));
    certificate.d->name = detail::stringOr(json, QStringLiteral("name"));
    certificate.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));

    const QJsonObject details = json.value(kDetails).toObject();
    certificate.d->validAt = detail::int64Or(details, QStringLiteral("valid_at"));
    certificate.d->expiresAt = detail::int64Or(details, QStringLiteral("expires_at"));
    certificate.d->pemContent = detail::stringOr(details, QStringLiteral("content"));

    certificate.d->active = detail::optionalBool(json, QStringLiteral("active"));
    return certificate;
}

bool Certificate::operator==(const Certificate &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->name == other.d->name
           && d->createdAt == other.d->createdAt && d->validAt == other.d->validAt
           && d->expiresAt == other.d->expiresAt && d->pemContent == other.d->pemContent
           && d->active == other.d->active;
}

} // namespace Core
} // namespace QtOpenAi
