// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/ImageEditRequest.h>
#include <QtOpenAi/Core/ImageGenerationRequest.h>
#include <QtOpenAi/Core/ImageResponse.h>
#include <QtOpenAi/Core/ImageVariationRequest.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the images types (#14): generation request serialisation,
// edit/variation multipart form-field construction, and response parsing.
class TestImages : public QObject
{
    Q_OBJECT
private slots:
    void generationRequiredAndOptional();
    void generationRoundTrip();
    void editFormFieldsAndFiles();
    void variationFormFields();
    void parsesB64Response();
    void parsesUrlResponseWithUsage();
};

static QString fieldValue(const QList<ImageEditRequest::FormField> &fields, const QString &name)
{
    for (const auto &field : fields)
        if (field.first == name)
            return field.second;
    return QString();
}

void TestImages::generationRequiredAndOptional()
{
    ImageGenerationRequest request(QStringLiteral("a red cube"), QStringLiteral("gpt-image-1"));
    // Only prompt/model set: nothing optional leaks.
    QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("prompt")).toString(), QStringLiteral("a red cube"));
    QCOMPARE(json.value(QStringLiteral("model")).toString(), QStringLiteral("gpt-image-1"));
    for (const QString &key :
         {QStringLiteral("n"), QStringLiteral("size"), QStringLiteral("quality"),
          QStringLiteral("response_format"), QStringLiteral("style"), QStringLiteral("background"),
          QStringLiteral("output_format"), QStringLiteral("moderation")}) {
        QVERIFY2(!json.contains(key), qPrintable(key));
    }

    request.setN(2);
    request.setSize(QStringLiteral("1024x1024"));
    request.setQuality(QStringLiteral("high"));
    request.setBackground(QStringLiteral("transparent"));
    json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("n")).toInt(), 2);
    QCOMPARE(json.value(QStringLiteral("size")).toString(), QStringLiteral("1024x1024"));
    QCOMPARE(json.value(QStringLiteral("quality")).toString(), QStringLiteral("high"));
    QCOMPARE(json.value(QStringLiteral("background")).toString(), QStringLiteral("transparent"));
}

void TestImages::generationRoundTrip()
{
    ImageGenerationRequest request(QStringLiteral("a cat"), QStringLiteral("dall-e-3"));
    request.setN(1);
    request.setSize(QStringLiteral("1792x1024"));
    request.setResponseFormat(QStringLiteral("b64_json"));
    request.setStyle(QStringLiteral("vivid"));
    request.setUser(QStringLiteral("u1"));

    const ImageGenerationRequest parsed = ImageGenerationRequest::fromJson(request.toJson());
    QCOMPARE(parsed, request);
}

void TestImages::editFormFieldsAndFiles()
{
    ImageEditRequest request(QByteArray("PNGsource"), QStringLiteral("in.png"),
                             QStringLiteral("add a hat"), QStringLiteral("gpt-image-1"));
    request.setMask(QStringLiteral("mask.png"), QByteArray("PNGmask"));
    request.setSize(QStringLiteral("1024x1024"));
    request.setN(3);

    const auto fields = request.formFields();
    QCOMPARE(fieldValue(fields, QStringLiteral("prompt")), QStringLiteral("add a hat"));
    QCOMPARE(fieldValue(fields, QStringLiteral("model")), QStringLiteral("gpt-image-1"));
    QCOMPARE(fieldValue(fields, QStringLiteral("size")), QStringLiteral("1024x1024"));
    QCOMPARE(fieldValue(fields, QStringLiteral("n")), QStringLiteral("3"));

    QCOMPARE(request.images().size(), 1);
    QCOMPARE(request.images().first().first, QStringLiteral("in.png"));
    QCOMPARE(request.images().first().second, QByteArray("PNGsource"));
    QVERIFY(request.hasMask());
    QCOMPARE(request.maskData(), QByteArray("PNGmask"));
}

void TestImages::variationFormFields()
{
    ImageVariationRequest request(QByteArray("PNG"), QStringLiteral("v.png"),
                                  QStringLiteral("dall-e-2"));
    request.setN(2);
    request.setResponseFormat(QStringLiteral("url"));

    const auto fields = request.formFields();
    QString model, n, rf;
    for (const auto &field : fields) {
        if (field.first == QStringLiteral("model"))
            model = field.second;
        else if (field.first == QStringLiteral("n"))
            n = field.second;
        else if (field.first == QStringLiteral("response_format"))
            rf = field.second;
    }
    QCOMPARE(model, QStringLiteral("dall-e-2"));
    QCOMPARE(n, QStringLiteral("2"));
    QCOMPARE(rf, QStringLiteral("url"));
    QCOMPARE(request.imageData(), QByteArray("PNG"));
}

void TestImages::parsesB64Response()
{
    const QJsonObject json {
            {QStringLiteral("created"), 1710000000},
            {QStringLiteral("data"),
             QJsonArray {QJsonObject {{QStringLiteral("b64_json"), QStringLiteral("aGVsbG8=")},
                                      {QStringLiteral("revised_prompt"),
                                       QStringLiteral("a red cube, studio lit")}}}}};

    const ImageResponse response = ImageResponse::fromJson(json);
    QCOMPARE(response.created(), Q_INT64_C(1710000000));
    QCOMPARE(response.data().size(), 1);
    QCOMPARE(response.firstImage().b64Json(), QStringLiteral("aGVsbG8="));
    QCOMPARE(response.firstImage().revisedPrompt(), QStringLiteral("a red cube, studio lit"));
    QVERIFY(!response.usage().has_value());
}

void TestImages::parsesUrlResponseWithUsage()
{
    const QJsonObject json {
            {QStringLiteral("created"), 1},
            {QStringLiteral("data"),
             QJsonArray {QJsonObject {{QStringLiteral("url"), QStringLiteral("https://x/y.png")}}}},
            {QStringLiteral("usage"), QJsonObject {{QStringLiteral("total_tokens"), 42}}}};

    const ImageResponse response = ImageResponse::fromJson(json);
    QCOMPARE(response.firstImage().url(), QStringLiteral("https://x/y.png"));
    QVERIFY(response.usage().has_value());
}

QTEST_APPLESS_MAIN(TestImages)
#include "tst_images.moc"
