// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/RealtimeClientSecret.h>
#include <QtOpenAi/Core/RealtimeEvent.h>
#include <QtOpenAi/Core/RealtimeSessionConfig.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the Realtime value types (#25): the session configuration that
// travels in both directions, the ephemeral client secret minted for a browser,
// and the client/server event envelope the WebSocket channel carries.
class TestRealtime : public QObject
{
    Q_OBJECT
private slots:
    void parsesSessionConfig();
    void sessionConfigRoundTrip();
    void sessionConfigKeepsInfiniteOutputTokens();
    void sessionConfigOmitsUnsetFields();
    void parsesClientSecret();
    void parsesNestedClientSecretSpelling();
    void clientSecretRoundTrip();
    void parsesServerEvent();
    void unmodelledEventFieldsSurviveRoundTrip();
    void decodesAudioDelta();
    void readsSessionFromSessionCreated();
    void readsErrorMessage();
    void buildsSessionUpdateEvent();
    void buildsAudioAppendEvent();
    void buildsUserMessageEvent();
    void buildsResponseCreateEvent();
};

namespace {

QJsonObject sampleAudio()
{
    return QJsonObject {
            {QStringLiteral("output"),
             QJsonObject {{QStringLiteral("voice"), QStringLiteral("alloy")},
                          {QStringLiteral("speed"), 1.0}}},
    };
}

} // namespace

void TestRealtime::parsesSessionConfig()
{
    const QJsonObject json {
            {QStringLiteral("type"), QStringLiteral("realtime")},
            {QStringLiteral("object"), QStringLiteral("realtime.session")},
            {QStringLiteral("id"), QStringLiteral("sess_123")},
            {QStringLiteral("model"), QStringLiteral("gpt-realtime")},
            {QStringLiteral("instructions"), QStringLiteral("Be brief.")},
            {QStringLiteral("output_modalities"), QJsonArray {QStringLiteral("audio")}},
            {QStringLiteral("audio"), sampleAudio()},
            {QStringLiteral("tools"),
             QJsonArray {QJsonObject {{QStringLiteral("type"), QStringLiteral("function")}}}},
            {QStringLiteral("tool_choice"), QStringLiteral("auto")},
            {QStringLiteral("max_output_tokens"), 4096},
            {QStringLiteral("expires_at"), 1756310470},
    };

    const RealtimeSessionConfig config = RealtimeSessionConfig::fromJson(json);
    QCOMPARE(config.type(), QStringLiteral("realtime"));
    QCOMPARE(config.object(), QStringLiteral("realtime.session"));
    QCOMPARE(config.id(), QStringLiteral("sess_123"));
    QCOMPARE(config.model(), QStringLiteral("gpt-realtime"));
    QCOMPARE(config.instructions(), QStringLiteral("Be brief."));
    QCOMPARE(config.outputModalities(), QStringList {QStringLiteral("audio")});
    // The audio tree is nested three levels deep and is carried verbatim.
    QCOMPARE(config.audio()
                     .value(QStringLiteral("output"))
                     .toObject()
                     .value(QStringLiteral("voice"))
                     .toString(),
             QStringLiteral("alloy"));
    QCOMPARE(config.tools().size(), 1);
    QCOMPARE(config.toolChoice().toString(), QStringLiteral("auto"));
    QCOMPARE(config.maxOutputTokens().toInt(), 4096);
    QCOMPARE(config.expiresAt(), Q_INT64_C(1756310470));
}

void TestRealtime::sessionConfigRoundTrip()
{
    RealtimeSessionConfig config;
    config.setType(QStringLiteral("realtime"));
    config.setObject(QStringLiteral("realtime.session"));
    config.setId(QStringLiteral("sess_123"));
    config.setModel(QStringLiteral("gpt-realtime"));
    config.setInstructions(QStringLiteral("Be brief."));
    config.setOutputModalities({QStringLiteral("text"), QStringLiteral("audio")});
    config.setAudio(sampleAudio());
    config.setTools(
            QJsonArray {QJsonObject {{QStringLiteral("type"), QStringLiteral("function")}}});
    config.setToolChoice(QStringLiteral("auto"));
    config.setMaxOutputTokens(4096);
    config.setTracing(QStringLiteral("auto"));
    config.setInclude({QStringLiteral("item.input_audio_transcription.logprobs")});
    config.setPrompt(QJsonObject {{QStringLiteral("id"), QStringLiteral("pmpt_1")}});
    config.setExpiresAt(1756310470);

    QCOMPARE(RealtimeSessionConfig::fromJson(config.toJson()), config);
}

void TestRealtime::sessionConfigKeepsInfiniteOutputTokens()
{
    // The API answers with the string "inf" as well as with a number, so the
    // field cannot be an int without losing one of the two.
    const QJsonObject json {
            {QStringLiteral("max_output_tokens"), QStringLiteral("inf")},
    };

    const RealtimeSessionConfig config = RealtimeSessionConfig::fromJson(json);
    QCOMPARE(config.maxOutputTokens().toString(), QStringLiteral("inf"));
    QCOMPARE(config.toJson().value(QStringLiteral("max_output_tokens")).toString(),
             QStringLiteral("inf"));
}

void TestRealtime::sessionConfigOmitsUnsetFields()
{
    RealtimeSessionConfig config;
    config.setModel(QStringLiteral("gpt-realtime"));

    // A session.update must carry only what it changes; anything else would
    // reset a field the caller never touched.
    QCOMPARE(config.toJson().keys(), QStringList {QStringLiteral("model")});
}

void TestRealtime::parsesClientSecret()
{
    const QJsonObject json {
            {QStringLiteral("value"), QStringLiteral("ek_68af296e")},
            {QStringLiteral("expires_at"), 1756310470},
            {QStringLiteral("session"),
             QJsonObject {{QStringLiteral("id"), QStringLiteral("sess_123")},
                          {QStringLiteral("model"), QStringLiteral("gpt-realtime")}}},
    };

    const RealtimeClientSecret secret = RealtimeClientSecret::fromJson(json);
    QCOMPARE(secret.value(), QStringLiteral("ek_68af296e"));
    QCOMPARE(secret.expiresAt(), Q_INT64_C(1756310470));
    QCOMPARE(secret.session().model(), QStringLiteral("gpt-realtime"));
}

void TestRealtime::parsesNestedClientSecretSpelling()
{
    // The pre-GA /realtime/transcription_sessions endpoint hangs the key off
    // the session instead of the other way round. Both spellings mean the same
    // thing, so both decode into this type.
    const QJsonObject json {
            {QStringLiteral("client_secret"),
             QJsonObject {{QStringLiteral("value"), QStringLiteral("ek_legacy")},
                          {QStringLiteral("expires_at"), 1756310470}}},
            {QStringLiteral("model"), QStringLiteral("gpt-4o-transcribe")},
    };

    const RealtimeClientSecret secret = RealtimeClientSecret::fromJson(json);
    QCOMPARE(secret.value(), QStringLiteral("ek_legacy"));
    QCOMPARE(secret.expiresAt(), Q_INT64_C(1756310470));
    QCOMPARE(secret.session().model(), QStringLiteral("gpt-4o-transcribe"));
}

void TestRealtime::clientSecretRoundTrip()
{
    RealtimeSessionConfig session;
    session.setId(QStringLiteral("sess_123"));
    session.setModel(QStringLiteral("gpt-realtime"));

    RealtimeClientSecret secret;
    secret.setValue(QStringLiteral("ek_68af296e"));
    secret.setExpiresAt(1756310470);
    secret.setSession(session);

    QCOMPARE(RealtimeClientSecret::fromJson(secret.toJson()), secret);
}

void TestRealtime::parsesServerEvent()
{
    const QJsonObject json {
            {QStringLiteral("event_id"), QStringLiteral("event_1")},
            {QStringLiteral("type"), QStringLiteral("response.output_text.delta")},
            {QStringLiteral("response_id"), QStringLiteral("resp_1")},
            {QStringLiteral("item_id"), QStringLiteral("item_1")},
            {QStringLiteral("delta"), QStringLiteral("Hel")},
    };

    const RealtimeEvent event = RealtimeEvent::fromJson(json);
    QCOMPARE(event.eventId(), QStringLiteral("event_1"));
    QCOMPARE(event.type(), QStringLiteral("response.output_text.delta"));
    QCOMPARE(event.responseId(), QStringLiteral("resp_1"));
    QCOMPARE(event.itemId(), QStringLiteral("item_1"));
    QCOMPARE(event.delta(), QStringLiteral("Hel"));
}

void TestRealtime::unmodelledEventFieldsSurviveRoundTrip()
{
    // The channel carries some 45 server events and 11 client ones. Only the
    // envelope is typed, so every payload has to survive verbatim.
    const QJsonObject json {
            {QStringLiteral("event_id"), QStringLiteral("event_2")},
            {QStringLiteral("type"), QStringLiteral("rate_limits.updated")},
            {QStringLiteral("rate_limits"),
             QJsonArray {QJsonObject {{QStringLiteral("name"), QStringLiteral("requests")},
                                      {QStringLiteral("remaining"), 99}}}},
    };

    const RealtimeEvent event = RealtimeEvent::fromJson(json);
    QCOMPARE(event.payload().value(QStringLiteral("rate_limits")).toArray().size(), 1);
    QCOMPARE(event.toJson(), json);
}

void TestRealtime::decodesAudioDelta()
{
    // Audio arrives base64-encoded inside the same `delta` field the text
    // events use, so the bytes are decoded on request rather than guessed at.
    const QByteArray pcm("\x01\x02\x03\x04", 4);
    const QJsonObject json {
            {QStringLiteral("type"), QStringLiteral("response.output_audio.delta")},
            {QStringLiteral("delta"), QString::fromLatin1(pcm.toBase64())},
    };

    const RealtimeEvent event = RealtimeEvent::fromJson(json);
    QCOMPARE(event.audioDelta(), pcm);
}

void TestRealtime::readsSessionFromSessionCreated()
{
    const QJsonObject json {
            {QStringLiteral("type"), QStringLiteral("session.created")},
            {QStringLiteral("session"),
             QJsonObject {{QStringLiteral("id"), QStringLiteral("sess_123")},
                          {QStringLiteral("model"), QStringLiteral("gpt-realtime")}}},
    };

    const RealtimeEvent event = RealtimeEvent::fromJson(json);
    QCOMPARE(event.session().id(), QStringLiteral("sess_123"));
    QCOMPARE(event.session().model(), QStringLiteral("gpt-realtime"));
}

void TestRealtime::readsErrorMessage()
{
    const QJsonObject json {
            {QStringLiteral("type"), QStringLiteral("error")},
            {QStringLiteral("error"),
             QJsonObject {{QStringLiteral("type"), QStringLiteral("invalid_request_error")},
                          {QStringLiteral("message"), QStringLiteral("Unknown parameter.")}}},
    };

    const RealtimeEvent event = RealtimeEvent::fromJson(json);
    QVERIFY(event.isError());
    QCOMPARE(event.errorMessage(), QStringLiteral("Unknown parameter."));
}

void TestRealtime::buildsSessionUpdateEvent()
{
    RealtimeSessionConfig config;
    config.setInstructions(QStringLiteral("Speak slowly."));

    const RealtimeEvent event = RealtimeEvent::sessionUpdate(config);
    QCOMPARE(event.type(), QStringLiteral("session.update"));
    QCOMPARE(event.toJson()
                     .value(QStringLiteral("session"))
                     .toObject()
                     .value(QStringLiteral("instructions"))
                     .toString(),
             QStringLiteral("Speak slowly."));
}

void TestRealtime::buildsAudioAppendEvent()
{
    const QByteArray pcm("\x01\x02\x03\x04", 4);

    const RealtimeEvent event = RealtimeEvent::appendInputAudio(pcm);
    QCOMPARE(event.type(), QStringLiteral("input_audio_buffer.append"));
    // The buffer field is base64 on the wire, and named `audio` rather than
    // `delta` — the one client event that does not mirror its server twin.
    QCOMPARE(event.toJson().value(QStringLiteral("audio")).toString(),
             QString::fromLatin1(pcm.toBase64()));
}

void TestRealtime::buildsUserMessageEvent()
{
    const RealtimeEvent event = RealtimeEvent::userMessage(QStringLiteral("Hello"));
    QCOMPARE(event.type(), QStringLiteral("conversation.item.create"));

    const QJsonObject item = event.toJson().value(QStringLiteral("item")).toObject();
    QCOMPARE(item.value(QStringLiteral("type")).toString(), QStringLiteral("message"));
    QCOMPARE(item.value(QStringLiteral("role")).toString(), QStringLiteral("user"));
    const QJsonObject part = item.value(QStringLiteral("content")).toArray().first().toObject();
    QCOMPARE(part.value(QStringLiteral("type")).toString(), QStringLiteral("input_text"));
    QCOMPARE(part.value(QStringLiteral("text")).toString(), QStringLiteral("Hello"));
}

void TestRealtime::buildsResponseCreateEvent()
{
    QCOMPARE(RealtimeEvent::createResponse().type(), QStringLiteral("response.create"));
    // Per-response overrides are optional and left out when empty.
    QVERIFY(!RealtimeEvent::createResponse().toJson().contains(QStringLiteral("response")));

    const RealtimeEvent overridden = RealtimeEvent::createResponse(
            QJsonObject {{QStringLiteral("instructions"), QStringLiteral("One word only.")}});
    QCOMPARE(overridden.toJson()
                     .value(QStringLiteral("response"))
                     .toObject()
                     .value(QStringLiteral("instructions"))
                     .toString(),
             QStringLiteral("One word only."));
}

QTEST_MAIN(TestRealtime)
#include "tst_realtime.moc"
