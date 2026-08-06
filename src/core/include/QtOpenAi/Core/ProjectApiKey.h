// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>
#include <QtOpenAi/Core/OrganizationUser.h>
#include <QtOpenAi/Core/ProjectServiceAccount.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class ApiKeyOwnerData;

// Who a project API key belongs to: either a person or a service account, never
// both and never neither.
//
// The API models this as a tagged union — `type` says which of the two sibling
// objects is filled in — and this keeps that shape rather than flattening it to
// one "owner name" string. Which kind of principal holds a key is the question
// an audit asks first, and a flattened field cannot answer it.
class QTOPENAI_CORE_EXPORT ApiKeyOwner
{
public:
    ApiKeyOwner();
    ApiKeyOwner(const ApiKeyOwner &other);
    ApiKeyOwner(ApiKeyOwner &&other) noexcept;
    ApiKeyOwner &operator=(const ApiKeyOwner &other);
    ApiKeyOwner &operator=(ApiKeyOwner &&other) noexcept;
    ~ApiKeyOwner();

    void swap(ApiKeyOwner &other) noexcept { d.swap(other.d); }

    // "user" or "service_account". A string, like every other discriminator on
    // this surface: a kind this build has never heard of has to survive.
    QString type() const;
    void setType(const QString &type);

    bool isUser() const { return type() == QLatin1String("user"); }
    bool isServiceAccount() const { return type() == QLatin1String("service_account"); }

    // Whichever of the two `type` names; the other is default-constructed.
    OrganizationUser user() const;
    void setUser(const OrganizationUser &user);

    ProjectServiceAccount serviceAccount() const;
    void setServiceAccount(const ProjectServiceAccount &serviceAccount);

    // The owner's display name whichever kind it is, for a list column that does
    // not care. Prefer the typed accessors when the distinction matters.
    QString name() const;

    QJsonObject toJson() const;
    static ApiKeyOwner fromJson(const QJsonObject &json);

    bool operator==(const ApiKeyOwner &other) const;
    bool operator!=(const ApiKeyOwner &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ApiKeyOwnerData> d;
};

class ProjectApiKeyData;

// An API key issued within a project (GET /organization/projects/{id}/api_keys,
// GET/DELETE .../{key_id}).
//
// **There is no `value`.** A project API key is only ever read back redacted;
// the secret exists once, at creation, and this endpoint family cannot create
// one — see ServiceAccountApiKey for the single place a secret is returned. That
// is the API's design and the right one: a listing that handed out live
// credentials would make the admin key a master key.
//
// The deletion acknowledgement decodes into this type as well, reporting the
// object as "organization.project.api_key.deleted".
class QTOPENAI_CORE_EXPORT ProjectApiKey
{
public:
    ProjectApiKey();
    ProjectApiKey(const ProjectApiKey &other);
    ProjectApiKey(ProjectApiKey &&other) noexcept;
    ProjectApiKey &operator=(const ProjectApiKey &other);
    ProjectApiKey &operator=(ProjectApiKey &&other) noexcept;
    ~ProjectApiKey();

    void swap(ProjectApiKey &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // Normally "organization.project.api_key".
    QString object() const;
    void setObject(const QString &object);

    QString name() const;
    void setName(const QString &name);

    // The key with its middle removed, e.g. "sk-abc...xyz". For recognising a
    // key in a list, not for using one.
    QString redactedValue() const;
    void setRedactedValue(const QString &redactedValue);

    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    // Unix timestamp of the last request made with this key, or 0 for a key that
    // has never been used — which is exactly what makes it worth revoking.
    qint64 lastUsedAt() const;
    void setLastUsedAt(qint64 lastUsedAt);

    ApiKeyOwner owner() const;
    void setOwner(const ApiKeyOwner &owner);

    QJsonObject toJson() const;
    static ProjectApiKey fromJson(const QJsonObject &json);

    bool operator==(const ProjectApiKey &other) const;
    bool operator!=(const ProjectApiKey &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ProjectApiKeyData> d;
};

using ProjectApiKeyList = ListPage<ProjectApiKey>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ApiKeyOwner)
Q_DECLARE_SHARED(QtOpenAi::Core::ProjectApiKey)
Q_DECLARE_METATYPE(QtOpenAi::Core::ApiKeyOwner)
Q_DECLARE_METATYPE(QtOpenAi::Core::ProjectApiKey)
Q_DECLARE_METATYPE(QtOpenAi::Core::ProjectApiKeyList)
