// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ProjectServiceAccount.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

QJsonObject ServiceAccountApiKey::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), object);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), name);
    detail::insertIfNotEmpty(json, QStringLiteral("value"), value);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), createdAt);
    return json;
}

ServiceAccountApiKey ServiceAccountApiKey::fromJson(const QJsonObject &json)
{
    ServiceAccountApiKey key;
    key.id = detail::stringOr(json, QStringLiteral("id"));
    key.object = detail::stringOr(json, QStringLiteral("object"));
    key.name = detail::stringOr(json, QStringLiteral("name"));
    key.value = detail::stringOr(json, QStringLiteral("value"));
    key.createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    return key;
}

class ProjectServiceAccountData : public QSharedData
{
public:
    QString id;
    QString object;
    QString name;
    QString role;
    qint64 createdAt = 0;
    ServiceAccountApiKey apiKey;
};

ProjectServiceAccount::ProjectServiceAccount()
    : d(new ProjectServiceAccountData)
{ }

ProjectServiceAccount::ProjectServiceAccount(const ProjectServiceAccount &other) = default;
ProjectServiceAccount::ProjectServiceAccount(ProjectServiceAccount &&other) noexcept = default;
ProjectServiceAccount &ProjectServiceAccount::operator=(const ProjectServiceAccount &other)
        = default;
ProjectServiceAccount &ProjectServiceAccount::operator=(ProjectServiceAccount &&other) noexcept
        = default;
ProjectServiceAccount::~ProjectServiceAccount() = default;

QString ProjectServiceAccount::id() const { return d->id; }
void ProjectServiceAccount::setId(const QString &id) { d->id = id; }

QString ProjectServiceAccount::object() const { return d->object; }
void ProjectServiceAccount::setObject(const QString &object) { d->object = object; }

QString ProjectServiceAccount::name() const { return d->name; }
void ProjectServiceAccount::setName(const QString &name) { d->name = name; }

QString ProjectServiceAccount::role() const { return d->role; }
void ProjectServiceAccount::setRole(const QString &role) { d->role = role; }

qint64 ProjectServiceAccount::createdAt() const { return d->createdAt; }
void ProjectServiceAccount::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

ServiceAccountApiKey ProjectServiceAccount::apiKey() const { return d->apiKey; }
void ProjectServiceAccount::setApiKey(const ServiceAccountApiKey &apiKey) { d->apiKey = apiKey; }

QJsonObject ProjectServiceAccount::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNotEmpty(json, QStringLiteral("role"), d->role);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    if (d->apiKey.isValid())
        json.insert(QStringLiteral("api_key"), d->apiKey.toJson());
    return json;
}

ProjectServiceAccount ProjectServiceAccount::fromJson(const QJsonObject &json)
{
    ProjectServiceAccount account;
    account.d->id = detail::stringOr(json, QStringLiteral("id"));
    account.d->object = detail::stringOr(json, QStringLiteral("object"));
    account.d->name = detail::stringOr(json, QStringLiteral("name"));
    account.d->role = detail::stringOr(json, QStringLiteral("role"));
    account.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    account.d->apiKey
            = ServiceAccountApiKey::fromJson(json.value(QStringLiteral("api_key")).toObject());
    return account;
}

bool ProjectServiceAccount::operator==(const ProjectServiceAccount &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->name == other.d->name
           && d->role == other.d->role && d->createdAt == other.d->createdAt
           && d->apiKey == other.d->apiKey;
}

} // namespace Core
} // namespace QtOpenAi
