// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/Enums.h>
#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class ChatKitSessionData;

// A ChatKit session (POST /chatkit/sessions, POST /chatkit/sessions/{id}/cancel)
// — the backend half of OpenAI's hosted chat UI.
//
// The reason to create one is clientSecret(): an ephemeral token the browser
// uses in place of the API key, scoped to one workflow, one end user and a short
// expiry. The key itself never leaves the server that mints this.
//
// `workflow` and `chatkit_configuration` are carried verbatim: the first is a
// reference plus arbitrary caller-defined state variables, the second a nested
// feature-toggle tree whose defaults the API fills in. Both are things a client
// forwards or inspects rather than acts on, so they stay raw rather than being
// half-modelled. `rate_limits` is not modelled at all — it carries the same
// number as maxRequestsPerMinute(), which the API documents as its convenience
// copy.
class QTOPENAI_CORE_EXPORT ChatKitSession
{
public:
    ChatKitSession();
    ChatKitSession(const ChatKitSession &other);
    ChatKitSession(ChatKitSession &&other) noexcept;
    ChatKitSession &operator=(const ChatKitSession &other);
    ChatKitSession &operator=(ChatKitSession &&other) noexcept;
    ~ChatKitSession();

    void swap(ChatKitSession &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "chatkit.session".
    QString object() const;
    void setObject(const QString &object);

    // Unix timestamp at which the session — and its client secret — expires.
    qint64 expiresAt() const;
    void setExpiresAt(qint64 expiresAt);

    // The ephemeral secret that authenticates the browser's session requests.
    QString clientSecret() const;
    void setClientSecret(const QString &clientSecret);

    // The end-user identifier the session is scoped to.
    QString user() const;
    void setUser(const QString &user);

    // The resolved per-minute request limit.
    int maxRequestsPerMinute() const;
    void setMaxRequestsPerMinute(int maxRequestsPerMinute);

    ChatKitSessionStatus status() const;
    void setStatus(ChatKitSessionStatus status);

    // The workflow backing the session (`workflow`), verbatim.
    QJsonObject workflow() const;
    void setWorkflow(const QJsonObject &workflow);

    // The resolved feature configuration (`chatkit_configuration`), verbatim.
    QJsonObject configuration() const;
    void setConfiguration(const QJsonObject &configuration);

    QJsonObject toJson() const;
    static ChatKitSession fromJson(const QJsonObject &json);

    bool operator==(const ChatKitSession &other) const;
    bool operator!=(const ChatKitSession &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ChatKitSessionData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ChatKitSession)
Q_DECLARE_METATYPE(QtOpenAi::Core::ChatKitSession)
