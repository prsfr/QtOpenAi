// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/CreateVoiceRequest.h>
#include <QtOpenAi/Core/Voice.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the custom-voice types (#12): the Voice and VoiceConsent value
// types, the consent list page, the deletion acknowledgement, and the multipart
// form fields of the two create requests.
class TestVoices : public QObject
{
    Q_OBJECT
private slots:
    void parsesVoice();
    void voiceRoundTrip();
    void parsesVoiceConsent();
    void voiceConsentRoundTrip();
    void parsesConsentDeletionAcknowledgement();
    void parsesConsentList();
    void voiceRequestFormFields();
    void consentRequestFormFields();
};

static QString fieldValue(const QList<QPair<QString, QString>> &fields, const QString &name)
{
    for (const auto &field : fields)
        if (field.first == name)
            return field.second;
    return QString();
}

void TestVoices::parsesVoice()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("voice_abc123")},
            {QStringLiteral("object"), QStringLiteral("audio.voice")},
            {QStringLiteral("created_at"), 1716028800},
            {QStringLiteral("name"), QStringLiteral("Narrator")},
            {QStringLiteral("voice_status"), QStringLiteral("ready")},
    };

    const Voice voice = Voice::fromJson(json);
    QCOMPARE(voice.id(), QStringLiteral("voice_abc123"));
    QCOMPARE(voice.object(), QStringLiteral("audio.voice"));
    QCOMPARE(voice.createdAt(), Q_INT64_C(1716028800));
    QCOMPARE(voice.name(), QStringLiteral("Narrator"));
    QCOMPARE(voice.voiceStatus(), QStringLiteral("ready"));
}

void TestVoices::voiceRoundTrip()
{
    Voice voice;
    voice.setId(QStringLiteral("voice_1"));
    voice.setObject(QStringLiteral("audio.voice"));
    voice.setCreatedAt(1700000000);
    voice.setName(QStringLiteral("Narrator"));
    voice.setVoiceStatus(QStringLiteral("processing"));

    QCOMPARE(Voice::fromJson(voice.toJson()), voice);
}

void TestVoices::parsesVoiceConsent()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("consent_abc123")},
            {QStringLiteral("object"), QStringLiteral("audio.voice_consent")},
            {QStringLiteral("created_at"), 1716028800},
            {QStringLiteral("name"), QStringLiteral("Jane Doe")},
            {QStringLiteral("language"), QStringLiteral("en")},
            {QStringLiteral("consent_status"), QStringLiteral("verified")},
    };

    const VoiceConsent consent = VoiceConsent::fromJson(json);
    QCOMPARE(consent.id(), QStringLiteral("consent_abc123"));
    QCOMPARE(consent.createdAt(), Q_INT64_C(1716028800));
    QCOMPARE(consent.name(), QStringLiteral("Jane Doe"));
    QCOMPARE(consent.language(), QStringLiteral("en"));
    QCOMPARE(consent.consentStatus(), QStringLiteral("verified"));
}

void TestVoices::voiceConsentRoundTrip()
{
    VoiceConsent consent;
    consent.setId(QStringLiteral("consent_1"));
    consent.setObject(QStringLiteral("audio.voice_consent"));
    consent.setCreatedAt(1700000000);
    consent.setName(QStringLiteral("Jane Doe"));
    consent.setLanguage(QStringLiteral("de"));
    consent.setConsentStatus(QStringLiteral("pending"));

    QCOMPARE(VoiceConsent::fromJson(consent.toJson()), consent);
}

void TestVoices::parsesConsentDeletionAcknowledgement()
{
    // DELETE answers with the same shape, minus the descriptive fields.
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("consent_1")},
            {QStringLiteral("object"), QStringLiteral("audio.voice_consent.deleted")},
            {QStringLiteral("deleted"), true},
    };

    const VoiceConsent consent = VoiceConsent::fromJson(json);
    QCOMPARE(consent.id(), QStringLiteral("consent_1"));
    QCOMPARE(consent.object(), QStringLiteral("audio.voice_consent.deleted"));
    QVERIFY(consent.name().isEmpty());
}

void TestVoices::parsesConsentList()
{
    const QJsonObject json {
            {QStringLiteral("object"), QStringLiteral("list")},
            {QStringLiteral("data"),
             QJsonArray {QJsonObject {{QStringLiteral("id"), QStringLiteral("consent_1")},
                                      {QStringLiteral("language"), QStringLiteral("en")}},
                         QJsonObject {{QStringLiteral("id"), QStringLiteral("consent_2")},
                                      {QStringLiteral("language"), QStringLiteral("de")}}}},
            {QStringLiteral("first_id"), QStringLiteral("consent_1")},
            {QStringLiteral("last_id"), QStringLiteral("consent_2")},
            {QStringLiteral("has_more"), false},
    };

    const VoiceConsentList list = VoiceConsentList::fromJson(json);
    QCOMPARE(list.size(), 2);
    QCOMPARE(list.data.at(1).language(), QStringLiteral("de"));
    QCOMPARE(list.lastId, QStringLiteral("consent_2"));
    QVERIFY(!list.hasMore);
    QCOMPARE(VoiceConsentList::fromJson(list.toJson()), list);
}

void TestVoices::voiceRequestFormFields()
{
    CreateVoiceRequest request(QStringLiteral("Narrator"), QStringLiteral("consent_1"),
                               QByteArray("RIFFfake"), QStringLiteral("sample.wav"));

    const auto fields = request.formFields();
    QCOMPARE(fieldValue(fields, QStringLiteral("name")), QStringLiteral("Narrator"));
    QCOMPARE(fieldValue(fields, QStringLiteral("consent")), QStringLiteral("consent_1"));
    // The sample travels out-of-band as the multipart file part.
    QCOMPARE(request.audioSample(), QByteArray("RIFFfake"));
    QCOMPARE(request.fileName(), QStringLiteral("sample.wav"));
}

void TestVoices::consentRequestFormFields()
{
    CreateVoiceConsentRequest request(QStringLiteral("Jane Doe"), QStringLiteral("en"),
                                      QByteArray("RIFFfake"), QStringLiteral("consent.wav"));

    const auto fields = request.formFields();
    QCOMPARE(fieldValue(fields, QStringLiteral("name")), QStringLiteral("Jane Doe"));
    QCOMPARE(fieldValue(fields, QStringLiteral("language")), QStringLiteral("en"));
    QCOMPARE(request.recording(), QByteArray("RIFFfake"));
    QCOMPARE(request.fileName(), QStringLiteral("consent.wav"));
}

QTEST_MAIN(TestVoices)
#include "tst_voices.moc"
