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

class VoiceData;

// A custom voice built from a recorded sample (POST /audio/voices), usable as
// the `voice` of a text-to-speech request once it finishes processing.
class QTOPENAI_CORE_EXPORT Voice
{
public:
    Voice();
    Voice(const Voice &other);
    Voice(Voice &&other) noexcept;
    Voice &operator=(const Voice &other);
    Voice &operator=(Voice &&other) noexcept;
    ~Voice();

    void swap(Voice &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "audio.voice".
    QString object() const;
    void setObject(const QString &object);

    // Unix creation timestamp (`created_at`); 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    QString name() const;
    void setName(const QString &name);

    // Processing state of the voice. Kept as a string: the value set is not
    // pinned down in the published spec, so provider values survive a
    // round-trip rather than collapsing into a guessed enum.
    QString voiceStatus() const;
    void setVoiceStatus(const QString &voiceStatus);

    QJsonObject toJson() const;
    static Voice fromJson(const QJsonObject &json);

    bool operator==(const Voice &other) const;
    bool operator!=(const Voice &other) const { return !(*this == other); }

private:
    QSharedDataPointer<VoiceData> d;
};

class VoiceConsentData;

// A recorded consent granting permission to clone a voice
// (/audio/voice_consents). A consent must exist and be accepted before
// POST /audio/voices will build a voice from a sample.
//
// The deletion acknowledgement of DELETE /audio/voice_consents/{id} shares this
// shape, with object() "audio.voice_consent.deleted".
class QTOPENAI_CORE_EXPORT VoiceConsent
{
public:
    VoiceConsent();
    VoiceConsent(const VoiceConsent &other);
    VoiceConsent(VoiceConsent &&other) noexcept;
    VoiceConsent &operator=(const VoiceConsent &other);
    VoiceConsent &operator=(VoiceConsent &&other) noexcept;
    ~VoiceConsent();

    void swap(VoiceConsent &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "audio.voice_consent".
    QString object() const;
    void setObject(const QString &object);

    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    // Name of the person who recorded the consent.
    QString name() const;
    void setName(const QString &name);

    // Language of the recording, e.g. "en".
    QString language() const;
    void setLanguage(const QString &language);

    // Verification state of the consent. A string for the same reason as
    // Voice::voiceStatus().
    QString consentStatus() const;
    void setConsentStatus(const QString &consentStatus);

    QJsonObject toJson() const;
    static VoiceConsent fromJson(const QJsonObject &json);

    bool operator==(const VoiceConsent &other) const;
    bool operator!=(const VoiceConsent &other) const { return !(*this == other); }

private:
    QSharedDataPointer<VoiceConsentData> d;
};

// A `list` of voice consents (GET /audio/voice_consents). Cursor-paginated;
// reuses the shared list-page type.
using VoiceConsentList = ListPage<VoiceConsent>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Voice)
Q_DECLARE_SHARED(QtOpenAi::Core::VoiceConsent)
Q_DECLARE_METATYPE(QtOpenAi::Core::Voice)
Q_DECLARE_METATYPE(QtOpenAi::Core::VoiceConsent)
Q_DECLARE_METATYPE(QtOpenAi::Core::VoiceConsentList)
