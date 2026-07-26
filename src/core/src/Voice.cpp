// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Voice.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

// --- Voice -----------------------------------------------------------------

class VoiceData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    QString name;
    QString voiceStatus;
};

Voice::Voice()
    : d(new VoiceData)
{ }

Voice::Voice(const Voice &other) = default;
Voice::Voice(Voice &&other) noexcept = default;
Voice &Voice::operator=(const Voice &other) = default;
Voice &Voice::operator=(Voice &&other) noexcept = default;
Voice::~Voice() = default;

QString Voice::id() const { return d->id; }
void Voice::setId(const QString &id) { d->id = id; }

QString Voice::object() const { return d->object; }
void Voice::setObject(const QString &object) { d->object = object; }

qint64 Voice::createdAt() const { return d->createdAt; }
void Voice::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString Voice::name() const { return d->name; }
void Voice::setName(const QString &name) { d->name = name; }

QString Voice::voiceStatus() const { return d->voiceStatus; }
void Voice::setVoiceStatus(const QString &voiceStatus) { d->voiceStatus = voiceStatus; }

QJsonObject Voice::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNotEmpty(json, QStringLiteral("voice_status"), d->voiceStatus);
    return json;
}

Voice Voice::fromJson(const QJsonObject &json)
{
    Voice voice;
    voice.d->id = detail::stringOr(json, QStringLiteral("id"));
    voice.d->object = detail::stringOr(json, QStringLiteral("object"));
    voice.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    voice.d->name = detail::stringOr(json, QStringLiteral("name"));
    voice.d->voiceStatus = detail::stringOr(json, QStringLiteral("voice_status"));
    return voice;
}

bool Voice::operator==(const Voice &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->name == other.d->name
           && d->voiceStatus == other.d->voiceStatus;
}

// --- VoiceConsent ----------------------------------------------------------

class VoiceConsentData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    QString name;
    QString language;
    QString consentStatus;
};

VoiceConsent::VoiceConsent()
    : d(new VoiceConsentData)
{ }

VoiceConsent::VoiceConsent(const VoiceConsent &other) = default;
VoiceConsent::VoiceConsent(VoiceConsent &&other) noexcept = default;
VoiceConsent &VoiceConsent::operator=(const VoiceConsent &other) = default;
VoiceConsent &VoiceConsent::operator=(VoiceConsent &&other) noexcept = default;
VoiceConsent::~VoiceConsent() = default;

QString VoiceConsent::id() const { return d->id; }
void VoiceConsent::setId(const QString &id) { d->id = id; }

QString VoiceConsent::object() const { return d->object; }
void VoiceConsent::setObject(const QString &object) { d->object = object; }

qint64 VoiceConsent::createdAt() const { return d->createdAt; }
void VoiceConsent::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString VoiceConsent::name() const { return d->name; }
void VoiceConsent::setName(const QString &name) { d->name = name; }

QString VoiceConsent::language() const { return d->language; }
void VoiceConsent::setLanguage(const QString &language) { d->language = language; }

QString VoiceConsent::consentStatus() const { return d->consentStatus; }
void VoiceConsent::setConsentStatus(const QString &consentStatus)
{
    d->consentStatus = consentStatus;
}

QJsonObject VoiceConsent::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNotEmpty(json, QStringLiteral("language"), d->language);
    detail::insertIfNotEmpty(json, QStringLiteral("consent_status"), d->consentStatus);
    return json;
}

VoiceConsent VoiceConsent::fromJson(const QJsonObject &json)
{
    VoiceConsent consent;
    consent.d->id = detail::stringOr(json, QStringLiteral("id"));
    consent.d->object = detail::stringOr(json, QStringLiteral("object"));
    consent.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    consent.d->name = detail::stringOr(json, QStringLiteral("name"));
    consent.d->language = detail::stringOr(json, QStringLiteral("language"));
    consent.d->consentStatus = detail::stringOr(json, QStringLiteral("consent_status"));
    return consent;
}

bool VoiceConsent::operator==(const VoiceConsent &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->name == other.d->name
           && d->language == other.d->language && d->consentStatus == other.d->consentStatus;
}

} // namespace Core
} // namespace QtOpenAi
