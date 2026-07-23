// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/ModerationRequest.h>
#include <QtOpenAi/Core/ModerationResponse.h>

#include <QtCore/QJsonDocument>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// JSON round-trip coverage for the Moderations value types.
class TestModerations : public QObject
{
    Q_OBJECT
private slots:
    void requestRoundTrip();
    void responseRoundTrip();
    void parsesResponse();
};

void TestModerations::requestRoundTrip()
{
    ModerationRequest request(QStringLiteral("I want to hurt someone"));
    request.setModel(QStringLiteral("omni-moderation-latest"));

    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("input")).toString(),
             QStringLiteral("I want to hurt someone"));
    QCOMPARE(json.value(QStringLiteral("model")).toString(),
             QStringLiteral("omni-moderation-latest"));

    const ModerationRequest parsed = ModerationRequest::fromJson(json);
    QCOMPARE(parsed, request);
}

void TestModerations::responseRoundTrip()
{
    ModerationResult result;
    result.setFlagged(true);
    result.setCategories({{QStringLiteral("violence"), true}, {QStringLiteral("hate"), false}});
    result.setCategoryScores({{QStringLiteral("violence"), 0.92}, {QStringLiteral("hate"), 0.01}});

    ModerationResponse response;
    response.setId(QStringLiteral("modr_1"));
    response.setModel(QStringLiteral("omni-moderation-latest"));
    response.setResults({result});

    const ModerationResponse parsed = ModerationResponse::fromJson(response.toJson());
    QCOMPARE(parsed, response);
    QVERIFY(parsed.firstResult().flagged());
    QVERIFY(parsed.firstResult().isFlagged(QStringLiteral("violence")));
    QCOMPARE(parsed.firstResult().score(QStringLiteral("violence")), 0.92);
}

void TestModerations::parsesResponse()
{
    const QByteArray body = R"({
        "id": "modr_9", "model": "omni-moderation-latest",
        "results": [{
            "flagged": true,
            "categories": {"violence": true, "harassment": false},
            "category_scores": {"violence": 0.87, "harassment": 0.02},
            "category_applied_input_types": {"violence": ["text"]}
        }]
    })";
    const ModerationResponse response
            = ModerationResponse::fromJson(QJsonDocument::fromJson(body).object());
    const ModerationResult result = response.firstResult();
    QVERIFY(result.flagged());
    QVERIFY(result.isFlagged(QStringLiteral("violence")));
    QVERIFY(!result.isFlagged(QStringLiteral("harassment")));
    QCOMPARE(result.score(QStringLiteral("violence")), 0.87);
}

QTEST_APPLESS_MAIN(TestModerations)
#include "tst_moderations.moc"
