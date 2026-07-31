// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ChatKitSession.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ChatKitSessionData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 expiresAt = 0;
    QString clientSecret;
    QString user;
    int maxRequestsPerMinute = 0;
    ChatKitSessionStatus status = ChatKitSessionStatus::Active;
    QJsonObject workflow;
    QJsonObject configuration;
};

ChatKitSession::ChatKitSession()
    : d(new ChatKitSessionData)
{ }

ChatKitSession::ChatKitSession(const ChatKitSession &other) = default;
ChatKitSession::ChatKitSession(ChatKitSession &&other) noexcept = default;
ChatKitSession &ChatKitSession::operator=(const ChatKitSession &other) = default;
ChatKitSession &ChatKitSession::operator=(ChatKitSession &&other) noexcept = default;
ChatKitSession::~ChatKitSession() = default;

QString ChatKitSession::id() const { return d->id; }
void ChatKitSession::setId(const QString &id) { d->id = id; }

QString ChatKitSession::object() const { return d->object; }
void ChatKitSession::setObject(const QString &object) { d->object = object; }

qint64 ChatKitSession::expiresAt() const { return d->expiresAt; }
void ChatKitSession::setExpiresAt(qint64 expiresAt) { d->expiresAt = expiresAt; }

QString ChatKitSession::clientSecret() const { return d->clientSecret; }
void ChatKitSession::setClientSecret(const QString &clientSecret)
{
    d->clientSecret = clientSecret;
}

QString ChatKitSession::user() const { return d->user; }
void ChatKitSession::setUser(const QString &user) { d->user = user; }

int ChatKitSession::maxRequestsPerMinute() const { return d->maxRequestsPerMinute; }
void ChatKitSession::setMaxRequestsPerMinute(int maxRequestsPerMinute)
{
    d->maxRequestsPerMinute = maxRequestsPerMinute;
}

ChatKitSessionStatus ChatKitSession::status() const { return d->status; }
void ChatKitSession::setStatus(ChatKitSessionStatus status) { d->status = status; }

QJsonObject ChatKitSession::workflow() const { return d->workflow; }
void ChatKitSession::setWorkflow(const QJsonObject &workflow) { d->workflow = workflow; }

QJsonObject ChatKitSession::configuration() const { return d->configuration; }
void ChatKitSession::setConfiguration(const QJsonObject &configuration)
{
    d->configuration = configuration;
}

QJsonObject ChatKitSession::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("expires_at"), d->expiresAt);
    detail::insertIfNotEmpty(json, QStringLiteral("client_secret"), d->clientSecret);
    detail::insertIfNotEmpty(json, QStringLiteral("user"), d->user);
    detail::insertIfNonZero(json, QStringLiteral("max_requests_per_1_minute"),
                            d->maxRequestsPerMinute);
    json.insert(QStringLiteral("status"), chatKitSessionStatusToString(d->status));
    if (!d->workflow.isEmpty())
        json.insert(QStringLiteral("workflow"), d->workflow);
    if (!d->configuration.isEmpty())
        json.insert(QStringLiteral("chatkit_configuration"), d->configuration);
    return json;
}

ChatKitSession ChatKitSession::fromJson(const QJsonObject &json)
{
    ChatKitSession session;
    session.d->id = detail::stringOr(json, QStringLiteral("id"));
    session.d->object = detail::stringOr(json, QStringLiteral("object"));
    session.d->expiresAt = detail::int64Or(json, QStringLiteral("expires_at"));
    session.d->clientSecret = detail::stringOr(json, QStringLiteral("client_secret"));
    session.d->user = detail::stringOr(json, QStringLiteral("user"));
    session.d->maxRequestsPerMinute
            = int(detail::int64Or(json, QStringLiteral("max_requests_per_1_minute")));
    session.d->status
            = chatKitSessionStatusFromString(detail::stringOr(json, QStringLiteral("status")));
    session.d->workflow = json.value(QStringLiteral("workflow")).toObject();
    session.d->configuration = json.value(QStringLiteral("chatkit_configuration")).toObject();
    return session;
}

bool ChatKitSession::operator==(const ChatKitSession &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->expiresAt == other.d->expiresAt && d->clientSecret == other.d->clientSecret
           && d->user == other.d->user && d->maxRequestsPerMinute == other.d->maxRequestsPerMinute
           && d->status == other.d->status && d->workflow == other.d->workflow
           && d->configuration == other.d->configuration;
}

} // namespace Core
} // namespace QtOpenAi
