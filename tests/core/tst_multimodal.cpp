// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/ContentPart.h>
#include <QtOpenAi/Core/Message.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for multimodal message content parts and assistant audio output (#10).
class TestMultimodal : public QObject
{
    Q_OBJECT
private slots:
    void plainTextUnchanged();
    void textPlusImageRoundTrip();
    void textPlusAudioRoundTrip();
    void filePartRoundTrip();
    void assistantAudioOutput();
    void parsesArrayContent();
};

void TestMultimodal::plainTextUnchanged()
{
    // A string-content message must serialise exactly as before.
    const Message message = Message::user(QStringLiteral("hello"));
    const QJsonObject json = message.toJson();
    QVERIFY(json.value(QStringLiteral("content")).isString());
    QCOMPARE(json.value(QStringLiteral("content")).toString(), QStringLiteral("hello"));
    QCOMPARE(Message::fromJson(json), message);
}

void TestMultimodal::textPlusImageRoundTrip()
{
    const Message message
            = Message::user({ContentPart::text(QStringLiteral("What is this?")),
                             ContentPart::imageUrl(QStringLiteral("https://example.com/cat.png"),
                                                   QStringLiteral("high"))});

    const QJsonObject json = message.toJson();
    QVERIFY(json.value(QStringLiteral("content")).isArray());
    QCOMPARE(json.value(QStringLiteral("content")).toArray().size(), 2);

    const Message parsed = Message::fromJson(json);
    QCOMPARE(parsed, message);
    QCOMPARE(parsed.contentParts().size(), 2);
    QCOMPARE(parsed.contentParts().at(1).imageUrl(), QStringLiteral("https://example.com/cat.png"));
    QCOMPARE(parsed.contentParts().at(1).imageDetail(), QStringLiteral("high"));
    // content() convenience returns the concatenated text parts.
    QCOMPARE(parsed.content(), QStringLiteral("What is this?"));
}

void TestMultimodal::textPlusAudioRoundTrip()
{
    const Message message = Message::user(
            {ContentPart::text(QStringLiteral("Transcribe:")),
             ContentPart::inputAudio(QStringLiteral("BASE64AUDIO"), QStringLiteral("wav"))});

    const Message parsed = Message::fromJson(message.toJson());
    QCOMPARE(parsed, message);
    QCOMPARE(parsed.contentParts().at(1).audioData(), QStringLiteral("BASE64AUDIO"));
    QCOMPARE(parsed.contentParts().at(1).audioFormat(), QStringLiteral("wav"));
}

void TestMultimodal::filePartRoundTrip()
{
    const Message message = Message::user({ContentPart::file(QStringLiteral("file_123"))});
    const Message parsed = Message::fromJson(message.toJson());
    QCOMPARE(parsed, message);
    QCOMPARE(parsed.contentParts().first().fileId(), QStringLiteral("file_123"));
}

void TestMultimodal::assistantAudioOutput()
{
    Message message(Role::Assistant, QString());
    message.setContent(QString());
    message.setAudioId(QStringLiteral("audio_1"));
    message.setAudioData(QStringLiteral("BASE64"));
    message.setAudioTranscript(QStringLiteral("Hello there"));

    const QJsonObject json = message.toJson();
    QVERIFY(json.contains(QStringLiteral("audio")));
    QCOMPARE(json.value(QStringLiteral("audio"))
                     .toObject()
                     .value(QStringLiteral("transcript"))
                     .toString(),
             QStringLiteral("Hello there"));

    const Message parsed = Message::fromJson(json);
    QCOMPARE(parsed.audioId(), QStringLiteral("audio_1"));
    QCOMPARE(parsed.audioTranscript(), QStringLiteral("Hello there"));
}

void TestMultimodal::parsesArrayContent()
{
    const QByteArray body = R"({"role":"user","content":[
        {"type":"text","text":"Look:"},
        {"type":"image_url","image_url":{"url":"data:image/png;base64,AAA","detail":"low"}}
    ]})";
    const Message message = Message::fromJson(QJsonDocument::fromJson(body).object());
    QCOMPARE(message.contentParts().size(), 2);
    QVERIFY(message.contentParts().first().isText());
    QCOMPARE(message.contentParts().at(1).imageUrl(), QStringLiteral("data:image/png;base64,AAA"));
    QCOMPARE(message.content(), QStringLiteral("Look:"));
}

QTEST_APPLESS_MAIN(TestMultimodal)
#include "tst_multimodal.moc"
