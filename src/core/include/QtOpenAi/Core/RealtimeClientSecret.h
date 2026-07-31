// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/RealtimeSessionConfig.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class RealtimeClientSecretData;

// An ephemeral Realtime credential (POST /realtime/client_secrets, and the same
// shape from /realtime/sessions and the translation variant).
//
// value() is a short-lived token — it looks like "ek_1234" — that a browser or
// mobile client can open a Realtime connection with, so the API key never
// leaves the server that minted it. session() is the configuration the secret
// carries: any session opened with it starts there, and the connected client
// can still adjust it with a `session.update`.
//
// The pre-GA /realtime/transcription_sessions endpoint spells the same thing
// the other way round — a nested `client_secret` object beside the session's
// own fields — so that shape is accepted as an alternative spelling rather than
// needing a second type. Its remaining fields are the pre-GA flat session
// layout, of which only what this library models is picked up; new code should
// use /realtime/client_secrets with a transcription session instead.
class QTOPENAI_CORE_EXPORT RealtimeClientSecret
{
public:
    RealtimeClientSecret();
    RealtimeClientSecret(const RealtimeClientSecret &other);
    RealtimeClientSecret(RealtimeClientSecret &&other) noexcept;
    RealtimeClientSecret &operator=(const RealtimeClientSecret &other);
    RealtimeClientSecret &operator=(RealtimeClientSecret &&other) noexcept;
    ~RealtimeClientSecret();

    void swap(RealtimeClientSecret &other) noexcept { d.swap(other.d); }

    // The token itself.
    QString value() const;
    void setValue(const QString &value);

    // Unix timestamp after which the secret can no longer open a session. A
    // session already running outlives it.
    qint64 expiresAt() const;
    void setExpiresAt(qint64 expiresAt);

    // The configuration sessions opened with this secret start from.
    RealtimeSessionConfig session() const;
    void setSession(const RealtimeSessionConfig &session);

    QJsonObject toJson() const;
    static RealtimeClientSecret fromJson(const QJsonObject &json);

    bool operator==(const RealtimeClientSecret &other) const;
    bool operator!=(const RealtimeClientSecret &other) const { return !(*this == other); }

private:
    QSharedDataPointer<RealtimeClientSecretData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::RealtimeClientSecret)
Q_DECLARE_METATYPE(QtOpenAi::Core::RealtimeClientSecret)
