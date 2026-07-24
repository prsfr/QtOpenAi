// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/SpeechRequest.h>

#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the text-to-speech request (#12): required fields always emit,
// optionals only when set, and the body survives a JSON round-trip.
class TestSpeech : public QObject
{
    Q_OBJECT
private slots:
    void requiredFieldsAlwaysEmitted();
    void omitsOptionalsWhenUnset();
    void emitsOptionalsWhenSet();
    void roundTrip();
};

void TestSpeech::requiredFieldsAlwaysEmitted()
{
    const SpeechRequest request(QStringLiteral("gpt-4o-mini-tts"), QStringLiteral("Hello there"),
                                QStringLiteral("alloy"));
    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("model")).toString(), QStringLiteral("gpt-4o-mini-tts"));
    QCOMPARE(json.value(QStringLiteral("input")).toString(), QStringLiteral("Hello there"));
    QCOMPARE(json.value(QStringLiteral("voice")).toString(), QStringLiteral("alloy"));
}

void TestSpeech::omitsOptionalsWhenUnset()
{
    const SpeechRequest request(QStringLiteral("gpt-4o-mini-tts"), QStringLiteral("hi"),
                                QStringLiteral("alloy"));
    const QJsonObject json = request.toJson();
    for (const QString &key : {QStringLiteral("response_format"), QStringLiteral("speed"),
                               QStringLiteral("instructions"), QStringLiteral("stream_format")}) {
        QVERIFY2(!json.contains(key), qPrintable(key));
    }
}

void TestSpeech::emitsOptionalsWhenSet()
{
    SpeechRequest request(QStringLiteral("gpt-4o-mini-tts"), QStringLiteral("hi"),
                          QStringLiteral("verse"));
    request.setResponseFormat(QStringLiteral("opus"));
    request.setSpeed(1.25);
    request.setInstructions(QStringLiteral("Speak cheerfully."));
    request.setStreamFormat(QStringLiteral("sse"));

    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("response_format")).toString(), QStringLiteral("opus"));
    QCOMPARE(json.value(QStringLiteral("speed")).toDouble(), 1.25);
    QCOMPARE(json.value(QStringLiteral("instructions")).toString(),
             QStringLiteral("Speak cheerfully."));
    QCOMPARE(json.value(QStringLiteral("stream_format")).toString(), QStringLiteral("sse"));
}

void TestSpeech::roundTrip()
{
    SpeechRequest request(QStringLiteral("gpt-4o-mini-tts"), QStringLiteral("round trip"),
                          QStringLiteral("coral"));
    request.setResponseFormat(QStringLiteral("wav"));
    request.setSpeed(0.75);
    request.setInstructions(QStringLiteral("Slow and clear."));

    const SpeechRequest parsed = SpeechRequest::fromJson(request.toJson());
    QCOMPARE(parsed, request);
}

QTEST_APPLESS_MAIN(TestSpeech)
#include "tst_speech.moc"
