// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/RealtimeSessionConfig.h>

#include <QtCore/QByteArray>
#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class RealtimeEventData;

// One message of the Realtime WebSocket channel, in either direction.
//
// The protocol is two discriminated unions — some 45 server events and 11
// client ones — that agree on an envelope (`type`, `event_id`) and nothing
// else. Modelling 56 classes would mean 56 places to update whenever the API
// grows one, and a client that silently drops whatever it does not know. So the
// envelope is typed, the rest of every event is carried verbatim in payload(),
// and the fields that recur across the union get a named accessor each:
// delta(), itemId(), responseId(), session(), errorMessage().
//
// The static factories build the client events a caller actually sends. Anything
// they do not cover can still be sent by constructing an event with its type and
// setting payload() — nothing here is a closed set.
//
// Deltas are worth one note: text, transcript, audio and tool arguments all
// arrive in a field called `delta`, and the audio one is base64. delta() returns
// it as sent; audioDelta() decodes it.
class QTOPENAI_CORE_EXPORT RealtimeEvent
{
public:
    RealtimeEvent();
    explicit RealtimeEvent(const QString &type);
    RealtimeEvent(const RealtimeEvent &other);
    RealtimeEvent(RealtimeEvent &&other) noexcept;
    RealtimeEvent &operator=(const RealtimeEvent &other);
    RealtimeEvent &operator=(RealtimeEvent &&other) noexcept;
    ~RealtimeEvent();

    void swap(RealtimeEvent &other) noexcept { d.swap(other.d); }

    // The event name, e.g. "session.created", "response.output_audio.delta".
    QString type() const;
    void setType(const QString &type);

    // Server-assigned id, or one the client sets to correlate an error back to
    // the event that caused it.
    QString eventId() const;
    void setEventId(const QString &eventId);

    // Everything outside the envelope, verbatim — the body of whichever event
    // this is.
    QJsonObject payload() const;
    void setPayload(const QJsonObject &payload);

    // --- Fields that recur across the union --------------------------------
    // The incremental chunk of any "*.delta" event, as sent.
    QString delta() const;
    // The same chunk decoded, for "response.output_audio.delta" (base64 PCM).
    QByteArray audioDelta() const;

    // The conversation item an event belongs to, when it names one.
    QString itemId() const;
    // The response an event belongs to, when it names one.
    QString responseId() const;

    // The session of a "session.created" / "session.updated" event.
    RealtimeSessionConfig session() const;

    bool isError() const { return type() == QLatin1String("error"); }
    // The human-readable message of an "error" event; empty otherwise.
    QString errorMessage() const;

    // --- The client events ------------------------------------------------
    // Reconfigure the running session; only the fields set on `config` travel.
    static RealtimeEvent sessionUpdate(const RealtimeSessionConfig &config);
    // Append captured audio to the input buffer (base64-encoded on the wire).
    static RealtimeEvent appendInputAudio(const QByteArray &audio);
    // Close the current input turn; needed only without server-side turn
    // detection, which commits for you.
    static RealtimeEvent commitInputAudio();
    // Drop whatever the input buffer holds.
    static RealtimeEvent clearInputAudio();
    // Add a conversation item verbatim (a message, a function-call output, ...).
    static RealtimeEvent createConversationItem(const QJsonObject &item);
    // The common case of the above: a text message from the user.
    static RealtimeEvent userMessage(const QString &text);
    // Ask the model to answer, optionally overriding the session for this one
    // response.
    static RealtimeEvent createResponse(const QJsonObject &overrides = {});
    // Interrupt the response in progress.
    static RealtimeEvent cancelResponse();

    QJsonObject toJson() const;
    static RealtimeEvent fromJson(const QJsonObject &json);

    bool operator==(const RealtimeEvent &other) const;
    bool operator!=(const RealtimeEvent &other) const { return !(*this == other); }

private:
    QSharedDataPointer<RealtimeEventData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::RealtimeEvent)
Q_DECLARE_METATYPE(QtOpenAi::Core::RealtimeEvent)
