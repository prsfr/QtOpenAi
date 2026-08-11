// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/AdminApiKey.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

namespace {

// The owner is the same *idea* as a project key's -- who holds this credential
// -- but not the same JSON. A project key nests the principal under the name of
// its kind, `{"type":"user","user":{...}}`, because it can be either a person or
// a service account. An admin key is always a person, so the API inlines the
// user's fields alongside the discriminator instead:
//
//     {"type":"user","object":"organization.user","id":"user_1", ...}
//
// Core::ApiKeyOwner is still the right type -- callers should not have to learn
// a second owner class to ask the same question, and `type()` keeps whatever the
// server actually said -- so the two encodings are reconciled here rather than
// by teaching the shared fromJson() a shape that only one endpoint sends.
ApiKeyOwner ownerFromFlatJson(const QJsonObject &json)
{
    ApiKeyOwner owner;
    if (json.isEmpty())
        return owner;
    owner.setType(detail::stringOr(json, QStringLiteral("type")));
    owner.setUser(OrganizationUser::fromJson(json));
    return owner;
}

QJsonObject ownerToFlatJson(const ApiKeyOwner &owner)
{
    QJsonObject json = owner.user().toJson();
    if (json.isEmpty() && owner.type().isEmpty())
        return {};
    detail::insertIfNotEmpty(json, QStringLiteral("type"), owner.type());
    return json;
}

} // namespace

class AdminApiKeyData : public QSharedData
{
public:
    QString id;
    QString object;
    QString name;
    QString value;
    QString redactedValue;
    qint64 createdAt = 0;
    qint64 expiresAt = 0;
    qint64 lastUsedAt = 0;
    ApiKeyOwner owner;
    bool deleted = false;
};

AdminApiKey::AdminApiKey()
    : d(new AdminApiKeyData)
{ }

AdminApiKey::AdminApiKey(const AdminApiKey &other) = default;
AdminApiKey::AdminApiKey(AdminApiKey &&other) noexcept = default;
AdminApiKey &AdminApiKey::operator=(const AdminApiKey &other) = default;
AdminApiKey &AdminApiKey::operator=(AdminApiKey &&other) noexcept = default;
AdminApiKey::~AdminApiKey() = default;

QString AdminApiKey::id() const { return d->id; }
void AdminApiKey::setId(const QString &id) { d->id = id; }

QString AdminApiKey::object() const { return d->object; }
void AdminApiKey::setObject(const QString &object) { d->object = object; }

QString AdminApiKey::name() const { return d->name; }
void AdminApiKey::setName(const QString &name) { d->name = name; }

QString AdminApiKey::value() const { return d->value; }
void AdminApiKey::setValue(const QString &value) { d->value = value; }

QString AdminApiKey::redactedValue() const { return d->redactedValue; }
void AdminApiKey::setRedactedValue(const QString &redactedValue)
{
    d->redactedValue = redactedValue;
}

qint64 AdminApiKey::createdAt() const { return d->createdAt; }
void AdminApiKey::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

qint64 AdminApiKey::expiresAt() const { return d->expiresAt; }
void AdminApiKey::setExpiresAt(qint64 expiresAt) { d->expiresAt = expiresAt; }

qint64 AdminApiKey::lastUsedAt() const { return d->lastUsedAt; }
void AdminApiKey::setLastUsedAt(qint64 lastUsedAt) { d->lastUsedAt = lastUsedAt; }

ApiKeyOwner AdminApiKey::owner() const { return d->owner; }
void AdminApiKey::setOwner(const ApiKeyOwner &owner) { d->owner = owner; }

bool AdminApiKey::isDeleted() const { return d->deleted; }
void AdminApiKey::setDeleted(bool deleted) { d->deleted = deleted; }

QJsonObject AdminApiKey::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    // Written when it is there, which is only ever on a key that came straight
    // from a create. Round-tripping it is what lets a caller hand the created
    // key to their own storage; nothing in this library logs it.
    detail::insertIfNotEmpty(json, QStringLiteral("value"), d->value);
    detail::insertIfNotEmpty(json, QStringLiteral("redacted_value"), d->redactedValue);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    // Both of these are left out rather than written as a zero. The API sends
    // null for "never expires" and "never used", and 1970 is a different claim
    // -- it would sort an unused key as the least recently used one.
    detail::insertIfNonZero(json, QStringLiteral("expires_at"), d->expiresAt);
    detail::insertIfNonZero(json, QStringLiteral("last_used_at"), d->lastUsedAt);
    const QJsonObject owner = ownerToFlatJson(d->owner);
    if (!owner.isEmpty())
        json.insert(QStringLiteral("owner"), owner);
    detail::insertIfTrue(json, QStringLiteral("deleted"), d->deleted);
    return json;
}

AdminApiKey AdminApiKey::fromJson(const QJsonObject &json)
{
    AdminApiKey key;
    key.d->id = detail::stringOr(json, QStringLiteral("id"));
    key.d->object = detail::stringOr(json, QStringLiteral("object"));
    key.d->name = detail::stringOr(json, QStringLiteral("name"));
    key.d->value = detail::stringOr(json, QStringLiteral("value"));
    key.d->redactedValue = detail::stringOr(json, QStringLiteral("redacted_value"));
    key.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    key.d->expiresAt = detail::int64Or(json, QStringLiteral("expires_at"));
    key.d->lastUsedAt = detail::int64Or(json, QStringLiteral("last_used_at"));
    key.d->owner = ownerFromFlatJson(json.value(QStringLiteral("owner")).toObject());
    key.d->deleted = json.value(QStringLiteral("deleted")).toBool();
    return key;
}

bool AdminApiKey::operator==(const AdminApiKey &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->name == other.d->name
           && d->value == other.d->value && d->redactedValue == other.d->redactedValue
           && d->createdAt == other.d->createdAt && d->expiresAt == other.d->expiresAt
           && d->lastUsedAt == other.d->lastUsedAt && d->owner == other.d->owner
           && d->deleted == other.d->deleted;
}

} // namespace Core
} // namespace QtOpenAi
