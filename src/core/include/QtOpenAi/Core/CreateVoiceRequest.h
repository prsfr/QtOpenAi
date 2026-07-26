// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class CreateVoiceRequestData;

// The body of a POST /audio/voices request (multipart/form-data): build a
// custom voice from an audio sample, backed by an accepted consent.
class QTOPENAI_CORE_EXPORT CreateVoiceRequest
{
public:
    // One multipart form field. The audio sample travels separately as the file
    // part, mirroring FileUploadRequest.
    using FormField = QPair<QString, QString>;

    CreateVoiceRequest();
    CreateVoiceRequest(QString name, QString consentId, QByteArray audioSample, QString fileName);
    CreateVoiceRequest(const CreateVoiceRequest &other);
    CreateVoiceRequest(CreateVoiceRequest &&other) noexcept;
    CreateVoiceRequest &operator=(const CreateVoiceRequest &other);
    CreateVoiceRequest &operator=(CreateVoiceRequest &&other) noexcept;
    ~CreateVoiceRequest();

    void swap(CreateVoiceRequest &other) noexcept { d.swap(other.d); }

    QString name() const;
    void setName(const QString &name);

    // Id of the VoiceConsent authorising this voice.
    QString consentId() const;
    void setConsentId(const QString &consentId);

    QByteArray audioSample() const;
    void setAudioSample(const QByteArray &audioSample);

    QString fileName() const;
    void setFileName(const QString &fileName);

    // The non-file form fields, in wire order.
    QList<FormField> formFields() const;

    bool operator==(const CreateVoiceRequest &other) const;
    bool operator!=(const CreateVoiceRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CreateVoiceRequestData> d;
};

class CreateVoiceConsentRequestData;

// The body of a POST /audio/voice_consents request (multipart/form-data): the
// spoken consent recording that authorises cloning a voice.
class QTOPENAI_CORE_EXPORT CreateVoiceConsentRequest
{
public:
    using FormField = QPair<QString, QString>;

    CreateVoiceConsentRequest();
    CreateVoiceConsentRequest(QString name, QString language, QByteArray recording,
                              QString fileName);
    CreateVoiceConsentRequest(const CreateVoiceConsentRequest &other);
    CreateVoiceConsentRequest(CreateVoiceConsentRequest &&other) noexcept;
    CreateVoiceConsentRequest &operator=(const CreateVoiceConsentRequest &other);
    CreateVoiceConsentRequest &operator=(CreateVoiceConsentRequest &&other) noexcept;
    ~CreateVoiceConsentRequest();

    void swap(CreateVoiceConsentRequest &other) noexcept { d.swap(other.d); }

    QString name() const;
    void setName(const QString &name);

    // Language of the recording, e.g. "en".
    QString language() const;
    void setLanguage(const QString &language);

    QByteArray recording() const;
    void setRecording(const QByteArray &recording);

    QString fileName() const;
    void setFileName(const QString &fileName);

    QList<FormField> formFields() const;

    bool operator==(const CreateVoiceConsentRequest &other) const;
    bool operator!=(const CreateVoiceConsentRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CreateVoiceConsentRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::CreateVoiceRequest)
Q_DECLARE_SHARED(QtOpenAi::Core::CreateVoiceConsentRequest)
