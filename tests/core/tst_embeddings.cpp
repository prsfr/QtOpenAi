// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/EmbeddingRequest.h>
#include <QtOpenAi/Core/EmbeddingResponse.h>

#include <QtCore/QJsonDocument>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// JSON round-trip coverage for the Embeddings value types.
class TestEmbeddings : public QObject
{
    Q_OBJECT
private slots:
    void requestRoundTrip();
    void requestOmitsUnsetOptionals();
    void responseRoundTrip();
    void parsesResponse();
    void parsesBase64Embedding();
    void base64AndFloatResponsesAreEqual();
    void malformedBase64GivesEmptyVector_data();
    void malformedBase64GivesEmptyVector();
    void nonFiniteBase64GivesEmptyVector_data();
    void nonFiniteBase64GivesEmptyVector();
};

void TestEmbeddings::requestRoundTrip()
{
    EmbeddingRequest request(QStringLiteral("text-embedding-3-small"), QStringLiteral("hello"));
    request.setDimensions(256);
    request.setEncodingFormat(QStringLiteral("float"));
    request.setUser(QStringLiteral("u1"));

    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("model")).toString(),
             QStringLiteral("text-embedding-3-small"));
    QCOMPARE(json.value(QStringLiteral("input")).toString(), QStringLiteral("hello"));
    QCOMPARE(json.value(QStringLiteral("dimensions")).toInt(), 256);

    const EmbeddingRequest parsed = EmbeddingRequest::fromJson(json);
    QCOMPARE(parsed, request);
}

void TestEmbeddings::requestOmitsUnsetOptionals()
{
    const EmbeddingRequest request(QStringLiteral("m"), QStringLiteral("hi"));
    const QJsonObject json = request.toJson();
    QVERIFY(!json.contains(QStringLiteral("dimensions")));
    QVERIFY(!json.contains(QStringLiteral("encoding_format")));
    QVERIFY(!json.contains(QStringLiteral("user")));
}

void TestEmbeddings::responseRoundTrip()
{
    Embedding embedding;
    embedding.setIndex(0);
    embedding.setVector({0.1, -0.2, 0.3});

    EmbeddingResponse response;
    response.setModel(QStringLiteral("text-embedding-3-small"));
    response.setData({embedding});

    Usage usage;
    usage.setPromptTokens(3);
    usage.setTotalTokens(3);
    response.setUsage(usage);

    const EmbeddingResponse parsed = EmbeddingResponse::fromJson(response.toJson());
    QCOMPARE(parsed, response);
    QCOMPARE(parsed.firstVector(), (QList<double> {0.1, -0.2, 0.3}));
}

void TestEmbeddings::parsesResponse()
{
    const QByteArray body = R"({
        "object": "list",
        "data": [{"object": "embedding", "index": 0, "embedding": [0.5, 0.25, -0.125]}],
        "model": "text-embedding-3-small",
        "usage": {"prompt_tokens": 2, "total_tokens": 2}
    })";
    const EmbeddingResponse response
            = EmbeddingResponse::fromJson(QJsonDocument::fromJson(body).object());
    QCOMPARE(response.data().size(), 1);
    QCOMPARE(response.firstVector(), (QList<double> {0.5, 0.25, -0.125}));
    QCOMPARE(response.usage().promptTokens(), 2);
}

// encoding_format "base64" returns each embedding as little-endian float32,
// base64-encoded, instead of an array; it decodes to the same doubles. These
// four values are exact in float32, so the two forms compare equal.
void TestEmbeddings::parsesBase64Embedding()
{
    const Embedding embedding = Embedding::fromJson(
            QJsonDocument::fromJson(R"({"index":0,"embedding":"AAAAPwAAgD4AAAC+AACAPw=="})")
                    .object());
    QCOMPARE(embedding.index(), 0);
    QCOMPARE(embedding.vector(), (QList<double> {0.5, 0.25, -0.125, 1.0}));

    // The length is the decoded byte count over four, whatever `dimensions` was.
    const Embedding single
            = Embedding::fromJson(QJsonDocument::fromJson(R"({"embedding":"AAAAPw=="})").object());
    QCOMPARE(single.vector(), (QList<double> {0.5}));
}

void TestEmbeddings::base64AndFloatResponsesAreEqual()
{
    const QByteArray floats = R"({"object":"list","data":[{"object":"embedding","index":0,
        "embedding":[0.5,0.25,-0.125,1.0]}],"model":"text-embedding-3-small",
        "usage":{"prompt_tokens":2,"total_tokens":2}})";
    const QByteArray base64 = R"({"object":"list","data":[{"object":"embedding","index":0,
        "embedding":"AAAAPwAAgD4AAAC+AACAPw=="}],"model":"text-embedding-3-small",
        "usage":{"prompt_tokens":2,"total_tokens":2}})";
    const EmbeddingResponse fromFloats
            = EmbeddingResponse::fromJson(QJsonDocument::fromJson(floats).object());
    const EmbeddingResponse fromBase64
            = EmbeddingResponse::fromJson(QJsonDocument::fromJson(base64).object());
    QCOMPARE(fromBase64.data().size(), 1);
    QCOMPARE(fromBase64, fromFloats);
}

// A broken base64 embedding is no vector at all, never a partial or garbage one.
void TestEmbeddings::malformedBase64GivesEmptyVector_data()
{
    QTest::addColumn<QByteArray>("embedding");
    // One bad character inside otherwise valid data: a lenient decoder would
    // skip it and return {0.5}.
    QTest::newRow("outside the alphabet") << QByteArray("AAAA!Pw==");
    QTest::newRow("not a whole float") << QByteArray("AAAAPwAAgD4AAAA=");
}

void TestEmbeddings::malformedBase64GivesEmptyVector()
{
    QFETCH(QByteArray, embedding);
    const QJsonObject json {{QStringLiteral("embedding"), QString::fromLatin1(embedding)}};
    QVERIFY(Embedding::fromJson(json).vector().isEmpty());
}

// A float array can never carry NaN or infinity (the JSON parser rejects
// them), but float32 bits can. Such a vector is garbage to every metric and
// toJson() writes it as nulls, so it is no vector at all, like a broken string.
void TestEmbeddings::nonFiniteBase64GivesEmptyVector_data()
{
    QTest::addColumn<QByteArray>("embedding");
    QTest::newRow("quiet NaN") << QByteArray("AADAfw==");
    QTest::newRow("signalling NaN") << QByteArray("AQCAfw==");
    QTest::newRow("+infinity") << QByteArray("AACAfw==");
    QTest::newRow("-infinity") << QByteArray("AACA/w==");
    QTest::newRow("NaN after a valid value") << QByteArray("AAAAPwAAwH8=");
}

void TestEmbeddings::nonFiniteBase64GivesEmptyVector()
{
    QFETCH(QByteArray, embedding);
    const QJsonObject json {{QStringLiteral("embedding"), QString::fromLatin1(embedding)}};
    QVERIFY(Embedding::fromJson(json).vector().isEmpty());
}

QTEST_APPLESS_MAIN(TestEmbeddings)
#include "tst_embeddings.moc"
