// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/TranscriptionRequest.h>
#include <QtOpenAi/Core/TranscriptionResponse.h>
#include <QtOpenAi/Core/TranslationRequest.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the speech-to-text types (#13): multipart form-field construction
// for transcription/translation requests and parsing of a verbose_json response.
class TestTranscriptions : public QObject
{
    Q_OBJECT
private slots:
    void transcriptionFormFields();
    void transcriptionOmitsUnsetFields();
    void translationFormFields();
    void parsesVerboseJsonWithSegments();
    void parsesPlainText();
    void responseRoundTrip();
};

static QString fieldValue(const QList<TranscriptionRequest::FormField> &fields, const QString &name)
{
    for (const auto &field : fields)
        if (field.first == name)
            return field.second;
    return QString();
}

void TestTranscriptions::transcriptionFormFields()
{
    TranscriptionRequest request(QByteArray("audiobytes"), QStringLiteral("clip.mp3"),
                                 QStringLiteral("whisper-1"));
    request.setLanguage(QStringLiteral("en"));
    request.setPrompt(QStringLiteral("hello"));
    request.setResponseFormat(QStringLiteral("verbose_json"));
    request.setTemperature(0.2);
    request.setTimestampGranularities({QStringLiteral("word"), QStringLiteral("segment")});

    const auto fields = request.formFields();
    QCOMPARE(fieldValue(fields, QStringLiteral("model")), QStringLiteral("whisper-1"));
    QCOMPARE(fieldValue(fields, QStringLiteral("language")), QStringLiteral("en"));
    QCOMPARE(fieldValue(fields, QStringLiteral("prompt")), QStringLiteral("hello"));
    QCOMPARE(fieldValue(fields, QStringLiteral("response_format")), QStringLiteral("verbose_json"));
    QCOMPARE(fieldValue(fields, QStringLiteral("temperature")), QStringLiteral("0.2"));

    // Array-valued fields are repeated with the name[] convention.
    int granularityCount = 0;
    for (const auto &field : fields)
        if (field.first == QStringLiteral("timestamp_granularities[]"))
            ++granularityCount;
    QCOMPARE(granularityCount, 2);

    // The file itself is carried out-of-band, not as a form field.
    QCOMPARE(request.fileData(), QByteArray("audiobytes"));
    QCOMPARE(request.fileName(), QStringLiteral("clip.mp3"));
}

void TestTranscriptions::transcriptionOmitsUnsetFields()
{
    TranscriptionRequest request(QByteArray("x"), QStringLiteral("a.wav"),
                                 QStringLiteral("whisper-1"));
    const auto fields = request.formFields();
    // Only `model` is mandatory; nothing else should appear.
    QCOMPARE(fields.size(), 1);
    QCOMPARE(fields.first().first, QStringLiteral("model"));
}

void TestTranscriptions::translationFormFields()
{
    TranslationRequest request(QByteArray("x"), QStringLiteral("de.mp3"),
                               QStringLiteral("whisper-1"));
    request.setResponseFormat(QStringLiteral("text"));
    request.setTemperature(0.0);

    const auto fields = request.formFields();
    QCOMPARE(fieldValue(fields, QStringLiteral("model")), QStringLiteral("whisper-1"));
    QCOMPARE(fieldValue(fields, QStringLiteral("response_format")), QStringLiteral("text"));
    QCOMPARE(fieldValue(fields, QStringLiteral("temperature")), QStringLiteral("0"));
    // Translation has no language field.
    QVERIFY(fieldValue(fields, QStringLiteral("language")).isEmpty());
}

void TestTranscriptions::parsesVerboseJsonWithSegments()
{
    const QJsonObject json {
            {QStringLiteral("task"), QStringLiteral("transcribe")},
            {QStringLiteral("language"), QStringLiteral("english")},
            {QStringLiteral("duration"), 2.5},
            {QStringLiteral("text"), QStringLiteral("Hello world")},
            {QStringLiteral("segments"),
             QJsonArray {QJsonObject {{QStringLiteral("id"), 0},
                                      {QStringLiteral("start"), 0.0},
                                      {QStringLiteral("end"), 2.5},
                                      {QStringLiteral("text"), QStringLiteral("Hello world")},
                                      {QStringLiteral("avg_logprob"), -0.3}}}},
            {QStringLiteral("words"),
             QJsonArray {QJsonObject {{QStringLiteral("word"), QStringLiteral("Hello")},
                                      {QStringLiteral("start"), 0.0},
                                      {QStringLiteral("end"), 1.0}}}}};

    const TranscriptionResponse response = TranscriptionResponse::fromJson(json);
    QCOMPARE(response.text(), QStringLiteral("Hello world"));
    QCOMPARE(response.language(), QStringLiteral("english"));
    QCOMPARE(response.duration(), 2.5);
    QCOMPARE(response.segments().size(), 1);
    QCOMPARE(response.segments().first().text(), QStringLiteral("Hello world"));
    QCOMPARE(response.segments().first().end(), 2.5);
    QCOMPARE(response.words().size(), 1);
    QCOMPARE(response.words().first().word(), QStringLiteral("Hello"));
}

void TestTranscriptions::parsesPlainText()
{
    const TranscriptionResponse response = TranscriptionResponse::fromText(QStringLiteral("plain"));
    QCOMPARE(response.text(), QStringLiteral("plain"));
    QVERIFY(response.segments().isEmpty());
}

void TestTranscriptions::responseRoundTrip()
{
    TranscriptionResponse response;
    response.setText(QStringLiteral("hi"));
    response.setLanguage(QStringLiteral("english"));
    response.setDuration(1.0);
    TranscriptionSegment segment;
    segment.setId(1);
    segment.setStart(0.0);
    segment.setEnd(1.0);
    segment.setText(QStringLiteral("hi"));
    response.setSegments({segment});

    const TranscriptionResponse parsed = TranscriptionResponse::fromJson(response.toJson());
    QCOMPARE(parsed, response);
}

QTEST_APPLESS_MAIN(TestTranscriptions)
#include "tst_transcriptions.moc"
