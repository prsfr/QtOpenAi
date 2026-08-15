// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/AuditLog.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class AuditLogUserData : public QSharedData
{
public:
    QString id;
    QString email;
};

AuditLogUser::AuditLogUser()
    : d(new AuditLogUserData)
{ }

AuditLogUser::AuditLogUser(const AuditLogUser &other) = default;
AuditLogUser::AuditLogUser(AuditLogUser &&other) noexcept = default;
AuditLogUser &AuditLogUser::operator=(const AuditLogUser &other) = default;
AuditLogUser &AuditLogUser::operator=(AuditLogUser &&other) noexcept = default;
AuditLogUser::~AuditLogUser() = default;

QString AuditLogUser::id() const { return d->id; }
void AuditLogUser::setId(const QString &id) { d->id = id; }

QString AuditLogUser::email() const { return d->email; }
void AuditLogUser::setEmail(const QString &email) { d->email = email; }

QJsonObject AuditLogUser::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("email"), d->email);
    return json;
}

AuditLogUser AuditLogUser::fromJson(const QJsonObject &json)
{
    AuditLogUser user;
    user.d->id = detail::stringOr(json, QStringLiteral("id"));
    user.d->email = detail::stringOr(json, QStringLiteral("email"));
    return user;
}

bool AuditLogUser::operator==(const AuditLogUser &other) const
{
    return d->id == other.d->id && d->email == other.d->email;
}

class AuditLogActorData : public QSharedData
{
public:
    QString type;
    AuditLogUser user;
    QString ipAddress;
    QString userAgent;
    QString apiKeyId;
    QString apiKeyType;
    QString serviceAccountId;
};

AuditLogActor::AuditLogActor()
    : d(new AuditLogActorData)
{ }

AuditLogActor::AuditLogActor(const AuditLogActor &other) = default;
AuditLogActor::AuditLogActor(AuditLogActor &&other) noexcept = default;
AuditLogActor &AuditLogActor::operator=(const AuditLogActor &other) = default;
AuditLogActor &AuditLogActor::operator=(AuditLogActor &&other) noexcept = default;
AuditLogActor::~AuditLogActor() = default;

QString AuditLogActor::type() const { return d->type; }
void AuditLogActor::setType(const QString &type) { d->type = type; }

AuditLogUser AuditLogActor::user() const { return d->user; }
void AuditLogActor::setUser(const AuditLogUser &user) { d->user = user; }

QString AuditLogActor::ipAddress() const { return d->ipAddress; }
void AuditLogActor::setIpAddress(const QString &ipAddress) { d->ipAddress = ipAddress; }

QString AuditLogActor::userAgent() const { return d->userAgent; }
void AuditLogActor::setUserAgent(const QString &userAgent) { d->userAgent = userAgent; }

QString AuditLogActor::apiKeyId() const { return d->apiKeyId; }
void AuditLogActor::setApiKeyId(const QString &apiKeyId) { d->apiKeyId = apiKeyId; }

QString AuditLogActor::apiKeyType() const { return d->apiKeyType; }
void AuditLogActor::setApiKeyType(const QString &apiKeyType) { d->apiKeyType = apiKeyType; }

QString AuditLogActor::serviceAccountId() const { return d->serviceAccountId; }
void AuditLogActor::setServiceAccountId(const QString &serviceAccountId)
{
    d->serviceAccountId = serviceAccountId;
}

QJsonObject AuditLogActor::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("type"), d->type);

    // Written back into whichever half the actor is, so the round trip produces
    // the nesting the server sent rather than a flattened version of it.
    QJsonObject session;
    detail::insertIfNotEmpty(session, QStringLiteral("ip_address"), d->ipAddress);
    detail::insertIfNotEmpty(session, QStringLiteral("user_agent"), d->userAgent);

    QJsonObject apiKey;
    detail::insertIfNotEmpty(apiKey, QStringLiteral("id"), d->apiKeyId);
    detail::insertIfNotEmpty(apiKey, QStringLiteral("type"), d->apiKeyType);
    if (!d->serviceAccountId.isEmpty()) {
        QJsonObject serviceAccount;
        serviceAccount.insert(QStringLiteral("id"), d->serviceAccountId);
        apiKey.insert(QStringLiteral("service_account"), serviceAccount);
    }

    // The one person can hang off either half, so it goes back where it came
    // from: under the api_key when this actor is one, under the session
    // otherwise.
    const QJsonObject user = d->user.toJson();
    if (!user.isEmpty()) {
        if (!apiKey.isEmpty())
            apiKey.insert(QStringLiteral("user"), user);
        else
            session.insert(QStringLiteral("user"), user);
    }

    if (!session.isEmpty())
        json.insert(QStringLiteral("session"), session);
    if (!apiKey.isEmpty())
        json.insert(QStringLiteral("api_key"), apiKey);
    return json;
}

AuditLogActor AuditLogActor::fromJson(const QJsonObject &json)
{
    AuditLogActor actor;
    actor.d->type = detail::stringOr(json, QStringLiteral("type"));

    const QJsonObject session = json.value(QStringLiteral("session")).toObject();
    actor.d->ipAddress = detail::stringOr(session, QStringLiteral("ip_address"));
    actor.d->userAgent = detail::stringOr(session, QStringLiteral("user_agent"));

    const QJsonObject apiKey = json.value(QStringLiteral("api_key")).toObject();
    actor.d->apiKeyId = detail::stringOr(apiKey, QStringLiteral("id"));
    actor.d->apiKeyType = detail::stringOr(apiKey, QStringLiteral("type"));
    actor.d->serviceAccountId = detail::stringOr(
            apiKey.value(QStringLiteral("service_account")).toObject(), QStringLiteral("id"));

    // Read from whichever half carries one -- see the header: "who was it" has
    // the same answer whichever route they came in by. A service-account key
    // has no person, and leaves this empty.
    const QJsonObject sessionUser = session.value(QStringLiteral("user")).toObject();
    actor.d->user = AuditLogUser::fromJson(
            sessionUser.isEmpty() ? apiKey.value(QStringLiteral("user")).toObject() : sessionUser);
    return actor;
}

bool AuditLogActor::operator==(const AuditLogActor &other) const
{
    return d->type == other.d->type && d->user == other.d->user
           && d->ipAddress == other.d->ipAddress && d->userAgent == other.d->userAgent
           && d->apiKeyId == other.d->apiKeyId && d->apiKeyType == other.d->apiKeyType
           && d->serviceAccountId == other.d->serviceAccountId;
}

class AuditLogData : public QSharedData
{
public:
    QString id;
    QString type;
    qint64 effectiveAt = 0;
    QString projectId;
    QString projectName;
    AuditLogActor actor;
    QString source;
    QJsonObject payload;
};

AuditLog::AuditLog()
    : d(new AuditLogData)
{ }

AuditLog::AuditLog(const AuditLog &other) = default;
AuditLog::AuditLog(AuditLog &&other) noexcept = default;
AuditLog &AuditLog::operator=(const AuditLog &other) = default;
AuditLog &AuditLog::operator=(AuditLog &&other) noexcept = default;
AuditLog::~AuditLog() = default;

QString AuditLog::id() const { return d->id; }
void AuditLog::setId(const QString &id) { d->id = id; }

QString AuditLog::type() const { return d->type; }
void AuditLog::setType(const QString &type) { d->type = type; }

qint64 AuditLog::effectiveAt() const { return d->effectiveAt; }
void AuditLog::setEffectiveAt(qint64 effectiveAt) { d->effectiveAt = effectiveAt; }

QString AuditLog::projectId() const { return d->projectId; }
void AuditLog::setProjectId(const QString &projectId) { d->projectId = projectId; }

QString AuditLog::projectName() const { return d->projectName; }
void AuditLog::setProjectName(const QString &projectName) { d->projectName = projectName; }

AuditLogActor AuditLog::actor() const { return d->actor; }
void AuditLog::setActor(const AuditLogActor &actor) { d->actor = actor; }

QString AuditLog::source() const { return d->source; }
void AuditLog::setSource(const QString &source) { d->source = source; }

QJsonObject AuditLog::payload() const { return d->payload; }
void AuditLog::setPayload(const QJsonObject &payload) { d->payload = payload; }

QString AuditLog::resourceId() const { return detail::stringOr(d->payload, QStringLiteral("id")); }

QJsonObject AuditLog::data() const { return d->payload.value(QStringLiteral("data")).toObject(); }

QJsonObject AuditLog::changesRequested() const
{
    return d->payload.value(QStringLiteral("changes_requested")).toObject();
}

QJsonObject AuditLog::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("type"), d->type);
    detail::insertIfNonZero(json, QStringLiteral("effective_at"), d->effectiveAt);
    detail::insertIfNotEmpty(json, QStringLiteral("source"), d->source);

    QJsonObject project;
    detail::insertIfNotEmpty(project, QStringLiteral("id"), d->projectId);
    detail::insertIfNotEmpty(project, QStringLiteral("name"), d->projectName);
    if (!project.isEmpty())
        json.insert(QStringLiteral("project"), project);

    const QJsonObject actor = d->actor.toJson();
    if (!actor.isEmpty())
        json.insert(QStringLiteral("actor"), actor);

    // Back under the key it was found beneath, which is the type's own name --
    // see the header. Without a type there is nowhere to put it, and an entry
    // with no type is not one this endpoint produces.
    if (!d->payload.isEmpty() && !d->type.isEmpty())
        json.insert(d->type, d->payload);
    return json;
}

AuditLog AuditLog::fromJson(const QJsonObject &json)
{
    AuditLog entry;
    entry.d->id = detail::stringOr(json, QStringLiteral("id"));
    entry.d->type = detail::stringOr(json, QStringLiteral("type"));
    entry.d->effectiveAt = detail::int64Or(json, QStringLiteral("effective_at"));
    entry.d->source = detail::stringOr(json, QStringLiteral("source"));

    const QJsonObject project = json.value(QStringLiteral("project")).toObject();
    entry.d->projectId = detail::stringOr(project, QStringLiteral("id"));
    entry.d->projectName = detail::stringOr(project, QStringLiteral("name"));

    entry.d->actor = AuditLogActor::fromJson(json.value(QStringLiteral("actor")).toObject());

    // The payload sits under a key named after the type, so it is found by
    // looking the type up rather than by knowing it. An event type this build
    // has never seen reads exactly as well as one it has.
    entry.d->payload = json.value(entry.d->type).toObject();
    return entry;
}

bool AuditLog::operator==(const AuditLog &other) const
{
    return d->id == other.d->id && d->type == other.d->type
           && d->effectiveAt == other.d->effectiveAt && d->projectId == other.d->projectId
           && d->projectName == other.d->projectName && d->actor == other.d->actor
           && d->source == other.d->source && d->payload == other.d->payload;
}

} // namespace Core
} // namespace QtOpenAi
