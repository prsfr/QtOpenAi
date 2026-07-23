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

QTEST_APPLESS_MAIN(TestEmbeddings)
#include "tst_embeddings.moc"
