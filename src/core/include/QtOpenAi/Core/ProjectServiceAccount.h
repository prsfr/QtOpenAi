// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

// The API key a service account is created with.
//
// **`value` is the only time the secret is ever returned.** Creating a service
// account is the one response that carries it; every later read of the same key
// reports it redacted (see ProjectApiKey). A caller that does not store it here
// has to delete the account and make another one.
//
// A small aggregate rather than an implicitly shared class, like CostAmount: it
// is only ever read as part of a ProjectServiceAccount.
struct QTOPENAI_CORE_EXPORT ServiceAccountApiKey
{
    QString id;
    QString object;
    QString name;
    QString value; // the secret, present only in the creation response
    qint64 createdAt = 0;

    // False for a service account read back later, which carries no key at all.
    bool isValid() const { return !id.isEmpty(); }

    QJsonObject toJson() const;
    static ServiceAccountApiKey fromJson(const QJsonObject &json);

    bool operator==(const ServiceAccountApiKey &other) const
    {
        return id == other.id && object == other.object && name == other.name
               && value == other.value && createdAt == other.createdAt;
    }
    bool operator!=(const ServiceAccountApiKey &other) const { return !(*this == other); }
};

class ProjectServiceAccountData;

// A service account within a project (GET/POST
// /organization/projects/{id}/service_accounts, GET/DELETE .../{account_id}).
//
// A bot rather than a person: it has a project role and an API key, but no email
// address and no organization membership. That is what it is for — an
// application's credential that survives the departure of whoever created it.
//
// The deletion acknowledgement decodes into this type as well, reporting the
// object as "organization.project.service_account.deleted".
class QTOPENAI_CORE_EXPORT ProjectServiceAccount
{
public:
    ProjectServiceAccount();
    ProjectServiceAccount(const ProjectServiceAccount &other);
    ProjectServiceAccount(ProjectServiceAccount &&other) noexcept;
    ProjectServiceAccount &operator=(const ProjectServiceAccount &other);
    ProjectServiceAccount &operator=(ProjectServiceAccount &&other) noexcept;
    ~ProjectServiceAccount();

    void swap(ProjectServiceAccount &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // Normally "organization.project.service_account".
    QString object() const;
    void setObject(const QString &object);

    QString name() const;
    void setName(const QString &name);

    // The **project** role, "owner" or "member" — not the organization role of
    // OrganizationUser. Kept as the string the server sent, as those are.
    QString role() const;
    void setRole(const QString &role);

    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    // Set only in the response to creating the account — see
    // ServiceAccountApiKey.
    ServiceAccountApiKey apiKey() const;
    void setApiKey(const ServiceAccountApiKey &apiKey);

    QJsonObject toJson() const;
    static ProjectServiceAccount fromJson(const QJsonObject &json);

    bool operator==(const ProjectServiceAccount &other) const;
    bool operator!=(const ProjectServiceAccount &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ProjectServiceAccountData> d;
};

using ProjectServiceAccountList = ListPage<ProjectServiceAccount>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ProjectServiceAccount)
Q_DECLARE_METATYPE(QtOpenAi::Core::ServiceAccountApiKey)
Q_DECLARE_METATYPE(QtOpenAi::Core::ProjectServiceAccount)
Q_DECLARE_METATYPE(QtOpenAi::Core::ProjectServiceAccountList)
