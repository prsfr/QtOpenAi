// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>
#include <QtOpenAi/Core/ProjectApiKey.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class AdminApiKeyData;

// An organization-level admin API key (GET/POST /organization/admin_api_keys,
// GET/DELETE .../{key_id}).
//
// Part of the administration surface, which uses an admin API key rather than a
// standard one — see QtOpenAi::Admin::Organization. Note what that means here:
// these are the credentials that read and write everything else in that
// surface, so this endpoint family issues the keys that issue the keys.
//
// **`value()` is the secret, and it is filled in exactly once.** The API returns
// it in the creation response and never again — no later list or read carries
// it, by design, because an endpoint that handed live credentials back on demand
// would turn one admin key into a master key. So an empty `value()` on a key
// that came from a list or a read is the API saying "not here", not a decode
// that went wrong:
//
//     Core::AdminApiKey created = reply->apiKey();
//     store(created.value());        // the only chance there will ever be
//     // ...
//     Core::AdminApiKey listed = page.data.at(0);
//     listed.value().isEmpty();      // true, and correct
//
// This is one type for both shapes rather than two, because that is what the
// API describes: `AdminApiKeyCreateResponse` is defined as an `AdminApiKey`
// plus the one extra field. Splitting it would have made every reader of a key
// choose a type based on where the key came from.
//
// **Nothing here writes `value()` to a log.** The library's own logging goes
// through Client::LoggingInterceptor, which redacts the JSON fields that carry
// a credential — `value` among them — before a body is written. Treat the
// secret the same way anywhere else it goes.
//
// `redactedValue()` is the other one: `sk-admin...def`, always present, safe to
// display. It is for recognising a key in a list, not for using one.
class QTOPENAI_CORE_EXPORT AdminApiKey
{
public:
    AdminApiKey();
    AdminApiKey(const AdminApiKey &other);
    AdminApiKey(AdminApiKey &&other) noexcept;
    AdminApiKey &operator=(const AdminApiKey &other);
    AdminApiKey &operator=(AdminApiKey &&other) noexcept;
    ~AdminApiKey();

    void swap(AdminApiKey &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // Normally "organization.admin_api_key" (or
    // "organization.admin_api_key.deleted").
    QString object() const;
    void setObject(const QString &object);

    QString name() const;
    void setName(const QString &name);

    // The secret. Non-empty only on the key returned by createAdminApiKey();
    // see the class note.
    QString value() const;
    void setValue(const QString &value);

    // Whether this key carries the secret, which is the same question as "did
    // this come from a create". Reads better than comparing value() to an empty
    // string at the call site, and says what the emptiness means.
    bool hasValue() const { return !value().isEmpty(); }

    // The key with its middle removed, e.g. "sk-admin...def". Always present.
    QString redactedValue() const;
    void setRedactedValue(const QString &redactedValue);

    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    // Unix timestamp of when the key expires, or 0 for a key that never does.
    // The API sends an explicit null for the latter, which is the same answer.
    qint64 expiresAt() const;
    void setExpiresAt(qint64 expiresAt);

    // Unix timestamp of the last request made with this key, or 0 for a key
    // that has never been used — which is exactly what makes it worth revoking.
    qint64 lastUsedAt() const;
    void setLastUsedAt(qint64 lastUsedAt);

    // Who the key belongs to — always a person for an admin key, where a
    // project key's owner may instead be a service account.
    //
    // Shares Core::ApiKeyOwner with the project keys even so, because a caller
    // should not have to learn a second owner class to ask the same question.
    // The two are not the same JSON, though: a project key nests the principal
    // under the name of its kind and an admin key inlines the user's fields
    // beside the discriminator, so this type reconciles the two encodings. For
    // an admin key that means owner().isUser() and owner().user(); the service
    // account half of the union stays default-constructed.
    ApiKeyOwner owner() const;
    void setOwner(const ApiKeyOwner &owner);

    // True in the answer to DELETE .../admin_api_keys/{id}, which reports the
    // object as "organization.admin_api_key.deleted".
    bool isDeleted() const;
    void setDeleted(bool deleted);

    QJsonObject toJson() const;
    static AdminApiKey fromJson(const QJsonObject &json);

    bool operator==(const AdminApiKey &other) const;
    bool operator!=(const AdminApiKey &other) const { return !(*this == other); }

private:
    QSharedDataPointer<AdminApiKeyData> d;
};

using AdminApiKeyList = ListPage<AdminApiKey>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::AdminApiKey)
Q_DECLARE_METATYPE(QtOpenAi::Core::AdminApiKey)
Q_DECLARE_METATYPE(QtOpenAi::Core::AdminApiKeyList)
