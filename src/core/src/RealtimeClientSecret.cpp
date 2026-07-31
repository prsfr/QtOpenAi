// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/RealtimeClientSecret.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class RealtimeClientSecretData : public QSharedData
{
public:
    QString value;
    qint64 expiresAt = 0;
    RealtimeSessionConfig session;
};

RealtimeClientSecret::RealtimeClientSecret()
    : d(new RealtimeClientSecretData)
{ }

RealtimeClientSecret::RealtimeClientSecret(const RealtimeClientSecret &other) = default;
RealtimeClientSecret::RealtimeClientSecret(RealtimeClientSecret &&other) noexcept = default;
RealtimeClientSecret &RealtimeClientSecret::operator=(const RealtimeClientSecret &other) = default;
RealtimeClientSecret &RealtimeClientSecret::operator=(RealtimeClientSecret &&other) noexcept
        = default;
RealtimeClientSecret::~RealtimeClientSecret() = default;

QString RealtimeClientSecret::value() const { return d->value; }
void RealtimeClientSecret::setValue(const QString &value) { d->value = value; }

qint64 RealtimeClientSecret::expiresAt() const { return d->expiresAt; }
void RealtimeClientSecret::setExpiresAt(qint64 expiresAt) { d->expiresAt = expiresAt; }

RealtimeSessionConfig RealtimeClientSecret::session() const { return d->session; }
void RealtimeClientSecret::setSession(const RealtimeSessionConfig &session)
{
    d->session = session;
}

QJsonObject RealtimeClientSecret::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("value"), d->value);
    detail::insertIfNonZero(json, QStringLiteral("expires_at"), d->expiresAt);
    const QJsonObject session = d->session.toJson();
    if (!session.isEmpty())
        json.insert(QStringLiteral("session"), session);
    return json;
}

RealtimeClientSecret RealtimeClientSecret::fromJson(const QJsonObject &json)
{
    RealtimeClientSecret secret;
    const QJsonValue nested = json.value(QStringLiteral("client_secret"));
    if (nested.isObject()) {
        // The pre-GA spelling: the key hangs off the session rather than the
        // other way round.
        const QJsonObject key = nested.toObject();
        secret.d->value = detail::stringOr(key, QStringLiteral("value"));
        secret.d->expiresAt = detail::int64Or(key, QStringLiteral("expires_at"));
        QJsonObject session = json;
        session.remove(QStringLiteral("client_secret"));
        secret.d->session = RealtimeSessionConfig::fromJson(session);
        return secret;
    }
    secret.d->value = detail::stringOr(json, QStringLiteral("value"));
    secret.d->expiresAt = detail::int64Or(json, QStringLiteral("expires_at"));
    secret.d->session
            = RealtimeSessionConfig::fromJson(json.value(QStringLiteral("session")).toObject());
    return secret;
}

bool RealtimeClientSecret::operator==(const RealtimeClientSecret &other) const
{
    return d->value == other.d->value && d->expiresAt == other.d->expiresAt
           && d->session == other.d->session;
}

} // namespace Core
} // namespace QtOpenAi
