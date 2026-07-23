// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/ChatCompletionRequest.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the extended Chat Completion request parameters (#11): each is
// emitted only when set, omitted otherwise, and survives a JSON round-trip.
class TestChatRequestParams : public QObject
{
    Q_OBJECT
private:
    static ChatCompletionRequest base()
    {
        return ChatCompletionRequest(QStringLiteral("gpt-4o"),
                                     {Message::user(QStringLiteral("hi"))});
    }

private slots:
    void omitsAllWhenUnset();
    void emitsWhenSet();
    void roundTrip();
};

void TestChatRequestParams::omitsAllWhenUnset()
{
    const QJsonObject json = base().toJson();
    for (const QString &key : {QStringLiteral("frequency_penalty"),
                               QStringLiteral("presence_penalty"),
                               QStringLiteral("logit_bias"),
                               QStringLiteral("seed"),
                               QStringLiteral("stop"),
                               QStringLiteral("logprobs"),
                               QStringLiteral("top_logprobs"),
                               QStringLiteral("stream_options"),
                               QStringLiteral("modalities"),
                               QStringLiteral("prediction"),
                               QStringLiteral("parallel_tool_calls"),
                               QStringLiteral("max_tokens"),
                               QStringLiteral("service_tier"),
                               QStringLiteral("store"),
                               QStringLiteral("metadata"),
                               QStringLiteral("user"),
                               QStringLiteral("safety_identifier"),
                               QStringLiteral("prompt_cache_key"),
                               QStringLiteral("reasoning_effort"),
                               QStringLiteral("web_search_options")}) {
        QVERIFY2(!json.contains(key), qPrintable(key));
    }
}

void TestChatRequestParams::emitsWhenSet()
{
    ChatCompletionRequest request = base();
    request.setFrequencyPenalty(0.5);
    request.setPresencePenalty(-0.3);
    request.setLogitBias(QJsonObject {{QStringLiteral("50256"), -100}});
    request.setSeed(42);
    request.setStop(QJsonArray {QStringLiteral("\n"), QStringLiteral("END")});
    request.setLogprobs(true);
    request.setTopLogprobs(5);
    request.setStreamOptions(QJsonObject {{QStringLiteral("include_usage"), true}});
    request.setModalities({QStringLiteral("text"), QStringLiteral("audio")});
    request.setParallelToolCalls(false);
    request.setMaxTokens(128);
    request.setServiceTier(QStringLiteral("auto"));
    request.setStore(true);
    request.setMetadata(QJsonObject {{QStringLiteral("k"), QStringLiteral("v")}});
    request.setUser(QStringLiteral("u1"));
    request.setSafetyIdentifier(QStringLiteral("safe"));
    request.setPromptCacheKey(QStringLiteral("cache"));
    request.setReasoningEffort(QStringLiteral("high"));
    request.setWebSearchOptions(
            QJsonObject {{QStringLiteral("search_context_size"), QStringLiteral("low")}});

    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("frequency_penalty")).toDouble(), 0.5);
    QCOMPARE(json.value(QStringLiteral("seed")).toInt(), 42);
    QCOMPARE(json.value(QStringLiteral("stop")).toArray().size(), 2);
    QVERIFY(json.value(QStringLiteral("stream_options"))
                    .toObject()
                    .value(QStringLiteral("include_usage"))
                    .toBool());
    QCOMPARE(json.value(QStringLiteral("modalities")).toArray().size(), 2);
    QCOMPARE(json.value(QStringLiteral("service_tier")).toString(), QStringLiteral("auto"));
    QCOMPARE(json.value(QStringLiteral("reasoning_effort")).toString(), QStringLiteral("high"));
    QVERIFY(json.contains(QStringLiteral("parallel_tool_calls")));
    QVERIFY(!json.value(QStringLiteral("parallel_tool_calls")).toBool());
}

void TestChatRequestParams::roundTrip()
{
    ChatCompletionRequest request = base();
    request.setFrequencyPenalty(0.5);
    request.setSeed(42);
    request.setStop(QStringLiteral("\n"));
    request.setModalities({QStringLiteral("text")});
    request.setStore(true);
    request.setMetadata(QJsonObject {{QStringLiteral("k"), QStringLiteral("v")}});
    request.setUser(QStringLiteral("u1"));
    request.setReasoningEffort(QStringLiteral("low"));
    request.setParallelToolCalls(true);
    request.setLogitBias(QJsonObject {{QStringLiteral("1"), 5}});

    const ChatCompletionRequest parsed = ChatCompletionRequest::fromJson(request.toJson());
    QCOMPARE(parsed, request);
}

QTEST_APPLESS_MAIN(TestChatRequestParams)
#include "tst_chatrequestparams.moc"
