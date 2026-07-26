// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/CreateVoiceRequest.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

// --- CreateVoiceRequest ----------------------------------------------------

class CreateVoiceRequestData : public QSharedData
{
public:
    QString name;
    QString consentId;
    QByteArray audioSample;
    QString fileName;
};

CreateVoiceRequest::CreateVoiceRequest()
    : d(new CreateVoiceRequestData)
{ }

CreateVoiceRequest::CreateVoiceRequest(QString name, QString consentId, QByteArray audioSample,
                                       QString fileName)
    : d(new CreateVoiceRequestData)
{
    d->name = std::move(name);
    d->consentId = std::move(consentId);
    d->audioSample = std::move(audioSample);
    d->fileName = std::move(fileName);
}

CreateVoiceRequest::CreateVoiceRequest(const CreateVoiceRequest &other) = default;
CreateVoiceRequest::CreateVoiceRequest(CreateVoiceRequest &&other) noexcept = default;
CreateVoiceRequest &CreateVoiceRequest::operator=(const CreateVoiceRequest &other) = default;
CreateVoiceRequest &CreateVoiceRequest::operator=(CreateVoiceRequest &&other) noexcept = default;
CreateVoiceRequest::~CreateVoiceRequest() = default;

QString CreateVoiceRequest::name() const { return d->name; }
void CreateVoiceRequest::setName(const QString &name) { d->name = name; }

QString CreateVoiceRequest::consentId() const { return d->consentId; }
void CreateVoiceRequest::setConsentId(const QString &consentId) { d->consentId = consentId; }

QByteArray CreateVoiceRequest::audioSample() const { return d->audioSample; }
void CreateVoiceRequest::setAudioSample(const QByteArray &audioSample)
{
    d->audioSample = audioSample;
}

QString CreateVoiceRequest::fileName() const { return d->fileName; }
void CreateVoiceRequest::setFileName(const QString &fileName) { d->fileName = fileName; }

QList<CreateVoiceRequest::FormField> CreateVoiceRequest::formFields() const
{
    QList<FormField> fields;
    fields.append({QStringLiteral("name"), d->name});
    fields.append({QStringLiteral("consent"), d->consentId});
    return fields;
}

bool CreateVoiceRequest::operator==(const CreateVoiceRequest &other) const
{
    return d->name == other.d->name && d->consentId == other.d->consentId
           && d->audioSample == other.d->audioSample && d->fileName == other.d->fileName;
}

// --- CreateVoiceConsentRequest ---------------------------------------------

class CreateVoiceConsentRequestData : public QSharedData
{
public:
    QString name;
    QString language;
    QByteArray recording;
    QString fileName;
};

CreateVoiceConsentRequest::CreateVoiceConsentRequest()
    : d(new CreateVoiceConsentRequestData)
{ }

CreateVoiceConsentRequest::CreateVoiceConsentRequest(QString name, QString language,
                                                     QByteArray recording, QString fileName)
    : d(new CreateVoiceConsentRequestData)
{
    d->name = std::move(name);
    d->language = std::move(language);
    d->recording = std::move(recording);
    d->fileName = std::move(fileName);
}

CreateVoiceConsentRequest::CreateVoiceConsentRequest(const CreateVoiceConsentRequest &other)
        = default;
CreateVoiceConsentRequest::CreateVoiceConsentRequest(CreateVoiceConsentRequest &&other) noexcept
        = default;
CreateVoiceConsentRequest &
CreateVoiceConsentRequest::operator=(const CreateVoiceConsentRequest &other)
        = default;
CreateVoiceConsentRequest &
CreateVoiceConsentRequest::operator=(CreateVoiceConsentRequest &&other) noexcept
        = default;
CreateVoiceConsentRequest::~CreateVoiceConsentRequest() = default;

QString CreateVoiceConsentRequest::name() const { return d->name; }
void CreateVoiceConsentRequest::setName(const QString &name) { d->name = name; }

QString CreateVoiceConsentRequest::language() const { return d->language; }
void CreateVoiceConsentRequest::setLanguage(const QString &language) { d->language = language; }

QByteArray CreateVoiceConsentRequest::recording() const { return d->recording; }
void CreateVoiceConsentRequest::setRecording(const QByteArray &recording)
{
    d->recording = recording;
}

QString CreateVoiceConsentRequest::fileName() const { return d->fileName; }
void CreateVoiceConsentRequest::setFileName(const QString &fileName) { d->fileName = fileName; }

QList<CreateVoiceConsentRequest::FormField> CreateVoiceConsentRequest::formFields() const
{
    QList<FormField> fields;
    fields.append({QStringLiteral("name"), d->name});
    fields.append({QStringLiteral("language"), d->language});
    return fields;
}

bool CreateVoiceConsentRequest::operator==(const CreateVoiceConsentRequest &other) const
{
    return d->name == other.d->name && d->language == other.d->language
           && d->recording == other.d->recording && d->fileName == other.d->fileName;
}

} // namespace Core
} // namespace QtOpenAi
