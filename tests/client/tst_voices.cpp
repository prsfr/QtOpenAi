// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/StubServer.h"

// Offline stub-server coverage for the custom-voice endpoints (#12): the two
// multipart create calls and the voice-consent CRUD.
class TestVoicesClient : public QObject
{
    Q_OBJECT
private slots:
    void createVoicePostsMultipart();
    void createConsentPostsMultipart();
    void listsConsentsWithPagination();
    void getParsesConsent();
    void updatePostsName();
    void deleteIssuesDeleteVerb();
};

void TestVoicesClient::createVoicePostsMultipart()
{
    StubServer server(QByteArray(R"({"id":"voice_1","object":"audio.voice",)"
                                 R"("name":"Narrator","voice_status":"processing"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateVoiceRequest request(QStringLiteral("Narrator"), QStringLiteral("consent_1"),
                               QByteArray("RIFFfake"), QStringLiteral("sample.wav"));

    VoiceReply *reply = client.createVoice(request);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/audio/voices "));
    QVERIFY(server.requestHeaders().toLower().contains("content-type: multipart/form-data;"));
    QVERIFY(server.requestBody().contains("name=\"audio_sample\"; filename=\"sample.wav\""));
    QVERIFY(server.requestBody().contains("name=\"consent\""));
    QVERIFY(server.requestBody().contains("consent_1"));
    QCOMPARE(reply->voice().voiceStatus(), QStringLiteral("processing"));
    delete reply;
}

void TestVoicesClient::createConsentPostsMultipart()
{
    StubServer server(QByteArray(R"({"id":"consent_1","object":"audio.voice_consent",)"
                                 R"("name":"Jane Doe","language":"en",)"
                                 R"("consent_status":"pending"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateVoiceConsentRequest request(QStringLiteral("Jane Doe"), QStringLiteral("en"),
                                      QByteArray("RIFFfake"), QStringLiteral("consent.wav"));

    VoiceConsentReply *reply = client.createVoiceConsent(request);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/audio/voice_consents "));
    QVERIFY(server.requestBody().contains("name=\"recording\"; filename=\"consent.wav\""));
    QVERIFY(server.requestBody().contains("name=\"language\""));
    QCOMPARE(reply->consent().consentStatus(), QStringLiteral("pending"));
    delete reply;
}

void TestVoicesClient::listsConsentsWithPagination()
{
    StubServer server(QByteArray(R"({"object":"list","data":[{"id":"consent_1"},)"
                                 R"({"id":"consent_2"}],"has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.limit = 5;
    VoiceConsentListReply *reply = client.listVoiceConsents(params);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/audio/voice_consents?"));
    QVERIFY(server.requestLine().contains("limit=5"));
    QCOMPARE(reply->list().size(), 2);
    delete reply;
}

void TestVoicesClient::getParsesConsent()
{
    StubServer server(QByteArray(R"({"id":"consent_1","language":"de",)"
                                 R"("created_at":1716028800,"consent_status":"verified"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    VoiceConsentReply *reply = client.getVoiceConsent(QStringLiteral("consent_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/audio/voice_consents/consent_1 "));
    QCOMPARE(reply->consent().language(), QStringLiteral("de"));
    QCOMPARE(reply->consent().createdAt(), Q_INT64_C(1716028800));
    delete reply;
}

void TestVoicesClient::updatePostsName()
{
    StubServer server(QByteArray(R"({"id":"consent_1","name":"Renamed"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    VoiceConsentReply *reply
            = client.updateVoiceConsent(QStringLiteral("consent_1"), QStringLiteral("Renamed"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/audio/voice_consents/consent_1 "));
    QVERIFY(server.requestBody().contains("\"name\":\"Renamed\""));
    QCOMPARE(reply->consent().name(), QStringLiteral("Renamed"));
    delete reply;
}

void TestVoicesClient::deleteIssuesDeleteVerb()
{
    StubServer server(QByteArray(R"({"id":"consent_1",)"
                                 R"("object":"audio.voice_consent.deleted","deleted":true})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    VoiceConsentReply *reply = client.deleteVoiceConsent(QStringLiteral("consent_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("DELETE /v1/audio/voice_consents/consent_1 "));
    QCOMPARE(reply->consent().object(), QStringLiteral("audio.voice_consent.deleted"));
    delete reply;
}

QTEST_MAIN(TestVoicesClient)
#include "tst_voices.moc"
