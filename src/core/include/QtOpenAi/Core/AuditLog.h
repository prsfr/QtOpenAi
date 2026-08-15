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

class AuditLogUserData;

// A person named in an audit entry: the one who acted, or the one an API key
// belongs to. Two fields, and the email is the one an investigation starts from.
class QTOPENAI_CORE_EXPORT AuditLogUser
{
public:
    AuditLogUser();
    AuditLogUser(const AuditLogUser &other);
    AuditLogUser(AuditLogUser &&other) noexcept;
    AuditLogUser &operator=(const AuditLogUser &other);
    AuditLogUser &operator=(AuditLogUser &&other) noexcept;
    ~AuditLogUser();

    void swap(AuditLogUser &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    QString email() const;
    void setEmail(const QString &email);

    bool isEmpty() const { return id().isEmpty() && email().isEmpty(); }

    QJsonObject toJson() const;
    static AuditLogUser fromJson(const QJsonObject &json);

    bool operator==(const AuditLogUser &other) const;
    bool operator!=(const AuditLogUser &other) const { return !(*this == other); }

private:
    QSharedDataPointer<AuditLogUserData> d;
};

class AuditLogActorData;

// Who performed the logged action: a signed-in session, or an API key.
//
// The API nests this two deep — an actor is a session or an api_key, and an
// api_key in turn belongs to a user or a service account — and this keeps that
// shape rather than flattening it to one name, for the same reason
// Core::ApiKeyOwner does: *which kind of principal* did the thing is the first
// question an audit asks.
//
// **user() answers across both halves**, because "who was it" usually has one
// answer whichever route they came in by: it is the session's user, or the
// user the API key belongs to, whichever is filled. A service-account key has
// no person behind it and leaves it empty, which is itself the answer.
class QTOPENAI_CORE_EXPORT AuditLogActor
{
public:
    AuditLogActor();
    AuditLogActor(const AuditLogActor &other);
    AuditLogActor(AuditLogActor &&other) noexcept;
    AuditLogActor &operator=(const AuditLogActor &other);
    AuditLogActor &operator=(AuditLogActor &&other) noexcept;
    ~AuditLogActor();

    void swap(AuditLogActor &other) noexcept { d.swap(other.d); }

    // "session" or "api_key". A string, as every discriminator on this surface
    // is: a kind this build has never heard of has to survive.
    QString type() const;
    void setType(const QString &type);

    bool isSession() const { return type() == QLatin1String("session"); }
    bool isApiKey() const { return type() == QLatin1String("api_key"); }

    // The person behind the action, from whichever half is filled. Empty when a
    // service account acted — see serviceAccountId().
    AuditLogUser user() const;
    void setUser(const AuditLogUser &user);

    // --- Session actors ----------------------------------------------------
    // Where the action came from. The one field an investigation reaches for
    // after the email, and empty for an API-key actor.
    QString ipAddress() const;
    void setIpAddress(const QString &ipAddress);

    // The browser that made the request. Not in the API's schema, but present
    // in the responses its own documentation shows, so it is decoded rather
    // than dropped -- an audit trail is the wrong place to discard evidence
    // because a schema forgot to mention it.
    QString userAgent() const;
    void setUserAgent(const QString &userAgent);

    // --- API-key actors ----------------------------------------------------
    QString apiKeyId() const;
    void setApiKeyId(const QString &apiKeyId);

    // "user" or "service_account": which kind of principal the key belongs to.
    QString apiKeyType() const;
    void setApiKeyType(const QString &apiKeyType);

    bool isServiceAccount() const { return apiKeyType() == QLatin1String("service_account"); }

    QString serviceAccountId() const;
    void setServiceAccountId(const QString &serviceAccountId);

    QJsonObject toJson() const;
    static AuditLogActor fromJson(const QJsonObject &json);

    bool operator==(const AuditLogActor &other) const;
    bool operator!=(const AuditLogActor &other) const { return !(*this == other); }

private:
    QSharedDataPointer<AuditLogActorData> d;
};

class AuditLogData;

// One entry in the organization's audit trail (GET /organization/audit_logs).
//
// Part of the administration surface, which uses an admin API key rather than a
// standard one — see QtOpenAi::Admin::Organization.
//
// **The payload is keyed by the event type, and that is why there is no enum.**
// The API describes ~55 event types, each with its own payload schema, and it
// does not nest them under a `data` field — it puts the payload under a key
// *named after the type*:
//
//     {"id": "audit_log-abc", "type": "project.archived", "effective_at": 172...,
//      "project.archived": {"id": "proj_abc"}}
//
// So finding the payload never requires knowing the type in advance: it is
// whatever sits under `type()`. Modelling 55 C++ structs would have been a large
// surface that goes stale the moment OpenAI adds the 56th — and it adds them
// without notice — whereas this way an event type this build has never heard of
// arrives whole and readable. That is the same bargain Core::UsageResult makes
// for a counter it does not know, taken to its conclusion: **every** payload is
// raw JSON, and the accessors below are the fields that recur across nearly all
// of them.
//
//     for (const Core::AuditLog &entry : page.data) {
//         entry.type();               // "project.archived", whatever it is
//         entry.resourceId();         // what it happened to
//         entry.payload();            // everything, always
//     }
//
// An entry is immutable history, so this type is read-mostly; the setters exist
// for round-tripping and for tests.
class QTOPENAI_CORE_EXPORT AuditLog
{
public:
    AuditLog();
    AuditLog(const AuditLog &other);
    AuditLog(AuditLog &&other) noexcept;
    AuditLog &operator=(const AuditLog &other);
    AuditLog &operator=(AuditLog &&other) noexcept;
    ~AuditLog();

    void swap(AuditLog &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The event type, e.g. "project.archived" or "api_key.updated" — and the
    // key the payload lives under. A string rather than an enum; see the class
    // note.
    QString type() const;
    void setType(const QString &type);

    // Unix seconds at which the action took effect. This is the field the
    // effectiveAt filters of Admin::AuditLogQuery narrow on.
    qint64 effectiveAt() const;
    void setEffectiveAt(qint64 effectiveAt);

    // The project the action was scoped to, empty for actions that were not.
    // Flattened out of the API's one-purpose `project` wrapper, which holds
    // nothing but these two.
    QString projectId() const;
    void setProjectId(const QString &projectId);

    QString projectName() const;
    void setProjectName(const QString &projectName);

    AuditLogActor actor() const;
    void setActor(const AuditLogActor &actor);

    // The authorization context the server recorded, when it recorded one.
    QString source() const;
    void setSource(const QString &source);

    // Everything the event carried, exactly as it arrived. Never empty for an
    // event this build can find a payload for, and the only complete answer for
    // an event type added after this build.
    QJsonObject payload() const;
    void setPayload(const QJsonObject &payload);

    // What the event happened to — the payload's `id`, which is also what
    // AuditLogQuery::resourceIds filters on. Empty for the few event types that
    // name no resource, such as "login.failed".
    QString resourceId() const;

    // The two sub-objects that recur across the payloads, and they are not the
    // same claim: `data` is what a creation event was given, `changes_requested`
    // is what an update event asked to change. Empty when the event has neither.
    QJsonObject data() const;
    QJsonObject changesRequested() const;

    QJsonObject toJson() const;
    static AuditLog fromJson(const QJsonObject &json);

    bool operator==(const AuditLog &other) const;
    bool operator!=(const AuditLog &other) const { return !(*this == other); }

private:
    QSharedDataPointer<AuditLogData> d;
};

using AuditLogList = ListPage<AuditLog>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::AuditLogUser)
Q_DECLARE_SHARED(QtOpenAi::Core::AuditLogActor)
Q_DECLARE_SHARED(QtOpenAi::Core::AuditLog)
Q_DECLARE_METATYPE(QtOpenAi::Core::AuditLogUser)
Q_DECLARE_METATYPE(QtOpenAi::Core::AuditLogActor)
Q_DECLARE_METATYPE(QtOpenAi::Core::AuditLog)
Q_DECLARE_METATYPE(QtOpenAi::Core::AuditLogList)
