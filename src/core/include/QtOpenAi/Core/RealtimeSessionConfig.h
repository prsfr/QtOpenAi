// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace QtOpenAi {
namespace Core {

class RealtimeSessionConfigData;

// The configuration of a Realtime session.
//
// One type rather than the usual request/response pair, because the Realtime
// API genuinely uses one shape in four places: the body of POST
// /realtime/client_secrets and /realtime/sessions, the `session` of the
// `session.update` client event, and the `session` reported back in
// `session.created`. Splitting it would mean converting between two identical
// types every time a session is reconfigured mid-call. id(), object() and
// expiresAt() are the fields only the server fills in.
//
// Only what a caller acts on is typed. `audio` is a three-level tree (input and
// output, each with a format, plus transcription, noise reduction and turn
// detection) whose defaults the server resolves, and `tools`, `tool_choice`,
// `tracing` and `prompt` are open unions, so all of them are carried verbatim
// rather than half-modelled.
//
// `max_output_tokens` is a QJsonValue because the API answers with the string
// "inf" as readily as with a number; forcing it into an int would lose one of
// the two.
//
// Anything left unset stays out of toJson() entirely — a `session.update` that
// serialised defaults would reset fields the caller never touched.
class QTOPENAI_CORE_EXPORT RealtimeSessionConfig
{
public:
    RealtimeSessionConfig();
    RealtimeSessionConfig(const RealtimeSessionConfig &other);
    RealtimeSessionConfig(RealtimeSessionConfig &&other) noexcept;
    RealtimeSessionConfig &operator=(const RealtimeSessionConfig &other);
    RealtimeSessionConfig &operator=(RealtimeSessionConfig &&other) noexcept;
    ~RealtimeSessionConfig();

    void swap(RealtimeSessionConfig &other) noexcept { d.swap(other.d); }

    // The session kind: "realtime" (the default surface) or "transcription".
    QString type() const;
    void setType(const QString &type);

    // Server-assigned; empty on a configuration the caller built.
    QString id() const;
    void setId(const QString &id);

    // The object type, normally "realtime.session" (server-assigned).
    QString object() const;
    void setObject(const QString &object);

    QString model() const;
    void setModel(const QString &model);

    // The system-level instructions the session runs with.
    QString instructions() const;
    void setInstructions(const QString &instructions);

    // What the model may answer with: "audio" (default, transcript included)
    // or "text". The API rejects asking for both at once.
    QStringList outputModalities() const;
    void setOutputModalities(const QStringList &outputModalities);

    // Input and output audio configuration (`audio`), verbatim.
    QJsonObject audio() const;
    void setAudio(const QJsonObject &audio);

    QJsonArray tools() const;
    void setTools(const QJsonArray &tools);

    // "auto"/"none"/"required" or a tool-choice object.
    QJsonValue toolChoice() const;
    void setToolChoice(const QJsonValue &toolChoice);

    // A token budget, or the string "inf".
    QJsonValue maxOutputTokens() const;
    void setMaxOutputTokens(const QJsonValue &maxOutputTokens);

    // Tracing configuration: "auto", a configuration object, or null.
    QJsonValue tracing() const;
    void setTracing(const QJsonValue &tracing);

    // Extra fields to include in server events (`include`).
    QStringList include() const;
    void setInclude(const QStringList &include);

    // A stored prompt reference (`prompt`), verbatim.
    QJsonObject prompt() const;
    void setPrompt(const QJsonObject &prompt);

    // Unix timestamp at which the session expires (server-assigned).
    qint64 expiresAt() const;
    void setExpiresAt(qint64 expiresAt);

    QJsonObject toJson() const;
    static RealtimeSessionConfig fromJson(const QJsonObject &json);

    bool operator==(const RealtimeSessionConfig &other) const;
    bool operator!=(const RealtimeSessionConfig &other) const { return !(*this == other); }

private:
    QSharedDataPointer<RealtimeSessionConfigData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::RealtimeSessionConfig)
Q_DECLARE_METATYPE(QtOpenAi::Core::RealtimeSessionConfig)
