// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ProjectApiKey.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ApiKeyOwnerData : public QSharedData
{
public:
    QString type;
    OrganizationUser user;
    ProjectServiceAccount serviceAccount;
};

ApiKeyOwner::ApiKeyOwner()
    : d(new ApiKeyOwnerData)
{ }

ApiKeyOwner::ApiKeyOwner(const ApiKeyOwner &other) = default;
ApiKeyOwner::ApiKeyOwner(ApiKeyOwner &&other) noexcept = default;
ApiKeyOwner &ApiKeyOwner::operator=(const ApiKeyOwner &other) = default;
ApiKeyOwner &ApiKeyOwner::operator=(ApiKeyOwner &&other) noexcept = default;
ApiKeyOwner::~ApiKeyOwner() = default;

QString ApiKeyOwner::type() const { return d->type; }
void ApiKeyOwner::setType(const QString &type) { d->type = type; }

OrganizationUser ApiKeyOwner::user() const { return d->user; }
void ApiKeyOwner::setUser(const OrganizationUser &user) { d->user = user; }

ProjectServiceAccount ApiKeyOwner::serviceAccount() const { return d->serviceAccount; }
void ApiKeyOwner::setServiceAccount(const ProjectServiceAccount &serviceAccount)
{
    d->serviceAccount = serviceAccount;
}

QString ApiKeyOwner::name() const
{
    // Whichever side is filled in, without asking the caller to branch. Falls
    // back to the other rather than to the empty string, so a `type` this build
    // does not recognise still shows the name the server sent.
    return d->user.name().isEmpty() ? d->serviceAccount.name() : d->user.name();
}

QJsonObject ApiKeyOwner::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("type"), d->type);
    // Only the side that has something in it: the API sends null for the other,
    // and an absent key says the same thing.
    const QJsonObject user = d->user.toJson();
    if (!user.isEmpty())
        json.insert(QStringLiteral("user"), user);
    const QJsonObject account = d->serviceAccount.toJson();
    if (!account.isEmpty())
        json.insert(QStringLiteral("service_account"), account);
    return json;
}

ApiKeyOwner ApiKeyOwner::fromJson(const QJsonObject &json)
{
    ApiKeyOwner owner;
    owner.d->type = detail::stringOr(json, QStringLiteral("type"));
    owner.d->user = OrganizationUser::fromJson(json.value(QStringLiteral("user")).toObject());
    owner.d->serviceAccount = ProjectServiceAccount::fromJson(
            json.value(QStringLiteral("service_account")).toObject());
    return owner;
}

bool ApiKeyOwner::operator==(const ApiKeyOwner &other) const
{
    return d->type == other.d->type && d->user == other.d->user
           && d->serviceAccount == other.d->serviceAccount;
}

class ProjectApiKeyData : public QSharedData
{
public:
    QString id;
    QString object;
    QString name;
    QString redactedValue;
    qint64 createdAt = 0;
    qint64 lastUsedAt = 0;
    ApiKeyOwner owner;
};

ProjectApiKey::ProjectApiKey()
    : d(new ProjectApiKeyData)
{ }

ProjectApiKey::ProjectApiKey(const ProjectApiKey &other) = default;
ProjectApiKey::ProjectApiKey(ProjectApiKey &&other) noexcept = default;
ProjectApiKey &ProjectApiKey::operator=(const ProjectApiKey &other) = default;
ProjectApiKey &ProjectApiKey::operator=(ProjectApiKey &&other) noexcept = default;
ProjectApiKey::~ProjectApiKey() = default;

QString ProjectApiKey::id() const { return d->id; }
void ProjectApiKey::setId(const QString &id) { d->id = id; }

QString ProjectApiKey::object() const { return d->object; }
void ProjectApiKey::setObject(const QString &object) { d->object = object; }

QString ProjectApiKey::name() const { return d->name; }
void ProjectApiKey::setName(const QString &name) { d->name = name; }

QString ProjectApiKey::redactedValue() const { return d->redactedValue; }
void ProjectApiKey::setRedactedValue(const QString &redactedValue)
{
    d->redactedValue = redactedValue;
}

qint64 ProjectApiKey::createdAt() const { return d->createdAt; }
void ProjectApiKey::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

qint64 ProjectApiKey::lastUsedAt() const { return d->lastUsedAt; }
void ProjectApiKey::setLastUsedAt(qint64 lastUsedAt) { d->lastUsedAt = lastUsedAt; }

ApiKeyOwner ProjectApiKey::owner() const { return d->owner; }
void ProjectApiKey::setOwner(const ApiKeyOwner &owner) { d->owner = owner; }

QJsonObject ProjectApiKey::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNotEmpty(json, QStringLiteral("redacted_value"), d->redactedValue);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    // Left out rather than written as a zero: a key that has never been used has
    // no last-use time, and 1970 would sort it as the oldest instead.
    detail::insertIfNonZero(json, QStringLiteral("last_used_at"), d->lastUsedAt);
    const QJsonObject owner = d->owner.toJson();
    if (!owner.isEmpty())
        json.insert(QStringLiteral("owner"), owner);
    return json;
}

ProjectApiKey ProjectApiKey::fromJson(const QJsonObject &json)
{
    ProjectApiKey key;
    key.d->id = detail::stringOr(json, QStringLiteral("id"));
    key.d->object = detail::stringOr(json, QStringLiteral("object"));
    key.d->name = detail::stringOr(json, QStringLiteral("name"));
    key.d->redactedValue = detail::stringOr(json, QStringLiteral("redacted_value"));
    key.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    key.d->lastUsedAt = detail::int64Or(json, QStringLiteral("last_used_at"));
    key.d->owner = ApiKeyOwner::fromJson(json.value(QStringLiteral("owner")).toObject());
    return key;
}

bool ProjectApiKey::operator==(const ProjectApiKey &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->name == other.d->name
           && d->redactedValue == other.d->redactedValue && d->createdAt == other.d->createdAt
           && d->lastUsedAt == other.d->lastUsedAt && d->owner == other.d->owner;
}

} // namespace Core
} // namespace QtOpenAi
