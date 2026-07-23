// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/CompletionRequest.h>
#include <QtOpenAi/Core/CompletionResponse.h>

#include <QtCore/QJsonDocument>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// JSON round-trip coverage for the legacy Completions value types.
class TestCompletions : public QObject
{
    Q_OBJECT
private slots:
    void requestRoundTrip();
    void requestOmitsUnsetOptionals();
    void responseRoundTrip();
    void parsesResponse();
};

void TestCompletions::requestRoundTrip()
{
    CompletionRequest request(QStringLiteral("gpt-3.5-turbo-instruct"), QStringLiteral("Once"));
    request.setMaxTokens(64);
    request.setTemperature(0.5);
    request.setTopP(0.9);
    request.setN(2);
    request.setEcho(true);
    request.setStop(QStringLiteral("\n"));
    request.setPresencePenalty(0.1);
    request.setFrequencyPenalty(0.2);
    request.setBestOf(3);
    request.setSeed(42);
    request.setSuffix(QStringLiteral(" END"));

    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("model")).toString(),
             QStringLiteral("gpt-3.5-turbo-instruct"));
    QCOMPARE(json.value(QStringLiteral("prompt")).toString(), QStringLiteral("Once"));
    QCOMPARE(json.value(QStringLiteral("max_tokens")).toInt(), 64);
    QCOMPARE(json.value(QStringLiteral("best_of")).toInt(), 3);

    const CompletionRequest parsed = CompletionRequest::fromJson(json);
    QCOMPARE(parsed, request);
}

void TestCompletions::requestOmitsUnsetOptionals()
{
    const CompletionRequest request(QStringLiteral("m"), QStringLiteral("hi"));
    const QJsonObject json = request.toJson();
    QVERIFY(!json.contains(QStringLiteral("max_tokens")));
    QVERIFY(!json.contains(QStringLiteral("temperature")));
    QVERIFY(!json.contains(QStringLiteral("stop")));
    QVERIFY(!json.contains(QStringLiteral("stream")));
}

void TestCompletions::responseRoundTrip()
{
    CompletionResponse response;
    response.setId(QStringLiteral("cmpl_1"));
    response.setCreated(1700000000);
    response.setModel(QStringLiteral("gpt-3.5-turbo-instruct"));

    CompletionChoice choice;
    choice.setText(QStringLiteral(" upon a time"));
    choice.setIndex(0);
    choice.setFinishReason(QStringLiteral("length"));
    response.setChoices({choice});

    Usage usage;
    usage.setPromptTokens(1);
    usage.setCompletionTokens(4);
    usage.setTotalTokens(5);
    response.setUsage(usage);

    const CompletionResponse parsed = CompletionResponse::fromJson(response.toJson());
    QCOMPARE(parsed, response);
    QCOMPARE(parsed.firstText(), QStringLiteral(" upon a time"));
}

void TestCompletions::parsesResponse()
{
    const QByteArray body = R"({
        "id": "cmpl_9", "object": "text_completion", "created": 1, "model": "davinci-002",
        "choices": [{"text": "Hello world", "index": 0, "logprobs": null,
                     "finish_reason": "stop"}],
        "usage": {"prompt_tokens": 2, "completion_tokens": 2, "total_tokens": 4}
    })";
    const CompletionResponse response
            = CompletionResponse::fromJson(QJsonDocument::fromJson(body).object());
    QCOMPARE(response.choices().size(), 1);
    QCOMPARE(response.firstText(), QStringLiteral("Hello world"));
    QCOMPARE(response.choices().first().finishReason(), QStringLiteral("stop"));
    QCOMPARE(response.usage().totalTokens(), 4);
}

QTEST_APPLESS_MAIN(TestCompletions)
#include "tst_completions.moc"
