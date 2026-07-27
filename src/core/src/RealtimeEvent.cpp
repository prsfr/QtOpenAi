// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/RealtimeEvent.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

namespace {

// The envelope every event of both unions shares. Everything else is the body
// of one variant and lives in payload(), so the split is spelled once rather
// than in both directions of the serialisation.
constexpr QLatin1String kTypeKey("type");
constexpr QLatin1String kEventIdKey("event_id");

// Build a client event whose whole body is one named field.
RealtimeEvent eventWith(const QString &type, const QString &key, const QJsonValue &value)
{
    RealtimeEvent event(type);
    event.setPayload(QJsonObject {{key, value}});
    return event;
}

} // namespace

class RealtimeEventData : public QSharedData
{
public:
    QString type;
    QString eventId;
    QJsonObject payload;
};

RealtimeEvent::RealtimeEvent()
    : d(new RealtimeEventData)
{ }

RealtimeEvent::RealtimeEvent(const QString &type)
    : d(new RealtimeEventData)
{
    d->type = type;
}

RealtimeEvent::RealtimeEvent(const RealtimeEvent &other) = default;
RealtimeEvent::RealtimeEvent(RealtimeEvent &&other) noexcept = default;
RealtimeEvent &RealtimeEvent::operator=(const RealtimeEvent &other) = default;
RealtimeEvent &RealtimeEvent::operator=(RealtimeEvent &&other) noexcept = default;
RealtimeEvent::~RealtimeEvent() = default;

QString RealtimeEvent::type() const { return d->type; }
void RealtimeEvent::setType(const QString &type) { d->type = type; }

QString RealtimeEvent::eventId() const { return d->eventId; }
void RealtimeEvent::setEventId(const QString &eventId) { d->eventId = eventId; }

QJsonObject RealtimeEvent::payload() const { return d->payload; }
void RealtimeEvent::setPayload(const QJsonObject &payload) { d->payload = payload; }

QString RealtimeEvent::delta() const
{
    return detail::stringOr(d->payload, QStringLiteral("delta"));
}

QByteArray RealtimeEvent::audioDelta() const { return QByteArray::fromBase64(delta().toLatin1()); }

QString RealtimeEvent::itemId() const
{
    return detail::stringOr(d->payload, QStringLiteral("item_id"));
}

QString RealtimeEvent::responseId() const
{
    return detail::stringOr(d->payload, QStringLiteral("response_id"));
}

RealtimeSessionConfig RealtimeEvent::session() const
{
    return RealtimeSessionConfig::fromJson(d->payload.value(QStringLiteral("session")).toObject());
}

QString RealtimeEvent::errorMessage() const
{
    return detail::stringOr(d->payload.value(QStringLiteral("error")).toObject(),
                            QStringLiteral("message"));
}

RealtimeEvent RealtimeEvent::sessionUpdate(const RealtimeSessionConfig &config)
{
    return eventWith(QStringLiteral("session.update"), QStringLiteral("session"), config.toJson());
}

RealtimeEvent RealtimeEvent::appendInputAudio(const QByteArray &audio)
{
    // The one client event whose payload field is not named like its server
    // twin: captured audio goes up as `audio`, deltas come back as `delta`.
    return eventWith(QStringLiteral("input_audio_buffer.append"), QStringLiteral("audio"),
                     QString::fromLatin1(audio.toBase64()));
}

RealtimeEvent RealtimeEvent::commitInputAudio()
{
    return RealtimeEvent(QStringLiteral("input_audio_buffer.commit"));
}

RealtimeEvent RealtimeEvent::clearInputAudio()
{
    return RealtimeEvent(QStringLiteral("input_audio_buffer.clear"));
}

RealtimeEvent RealtimeEvent::createConversationItem(const QJsonObject &item)
{
    return eventWith(QStringLiteral("conversation.item.create"), QStringLiteral("item"), item);
}

RealtimeEvent RealtimeEvent::userMessage(const QString &text)
{
    const QJsonObject part {
            {QStringLiteral("type"), QStringLiteral("input_text")},
            {QStringLiteral("text"), text},
    };
    return createConversationItem(QJsonObject {
            {QStringLiteral("type"), QStringLiteral("message")},
            {QStringLiteral("role"), QStringLiteral("user")},
            {QStringLiteral("content"), QJsonArray {part}},
    });
}

RealtimeEvent RealtimeEvent::createResponse(const QJsonObject &overrides)
{
    if (overrides.isEmpty())
        return RealtimeEvent(QStringLiteral("response.create"));
    return eventWith(QStringLiteral("response.create"), QStringLiteral("response"), overrides);
}

RealtimeEvent RealtimeEvent::cancelResponse()
{
    return RealtimeEvent(QStringLiteral("response.cancel"));
}

QJsonObject RealtimeEvent::toJson() const
{
    QJsonObject json = d->payload;
    detail::insertIfNotEmpty(json, kEventIdKey, d->eventId);
    detail::insertIfNotEmpty(json, kTypeKey, d->type);
    return json;
}

RealtimeEvent RealtimeEvent::fromJson(const QJsonObject &json)
{
    RealtimeEvent event;
    event.d->type = detail::stringOr(json, kTypeKey);
    event.d->eventId = detail::stringOr(json, kEventIdKey);
    event.d->payload = json;
    event.d->payload.remove(kTypeKey);
    event.d->payload.remove(kEventIdKey);
    return event;
}

bool RealtimeEvent::operator==(const RealtimeEvent &other) const
{
    return d->type == other.d->type && d->eventId == other.d->eventId
           && d->payload == other.d->payload;
}

} // namespace Core
} // namespace QtOpenAi
