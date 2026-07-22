// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/ChatCompletionRequest.h>
#include <QtOpenAi/Core/ChatCompletionResponse.h>
#include <QtOpenAi/Core/Message.h>
#include <QtOpenAi/Core/Tool.h>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Verifies JSON (de)serialisation against the OpenAI wire format, including
// tool-call round trips.
class TestSerialization : public QObject
{
    Q_OBJECT
private slots:
    void roleAndFinishReasonMapping();
    void messageRoundTrip();
    void assistantToolCallSerialization();
    void requestSerialization();
    void requestOmitsUnsetOptionals();
    void responseParsingFromSpecExample();
    void responseWithToolCalls();
};

void TestSerialization::roleAndFinishReasonMapping()
{
    QCOMPARE(roleToString(Role::Assistant), QStringLiteral("assistant"));
    QCOMPARE(roleFromString(QStringLiteral("tool")), Role::Tool);
    QCOMPARE(roleFromString(QStringLiteral("nonsense")), Role::User);

    QCOMPARE(finishReasonToString(FinishReason::ToolCalls), QStringLiteral("tool_calls"));
    QCOMPARE(finishReasonFromString(QStringLiteral("length")), FinishReason::Length);
    QVERIFY(finishReasonToString(FinishReason::None).isEmpty());
}

void TestSerialization::messageRoundTrip()
{
    Message message = Message::user(QStringLiteral("Hello"));
    message.setName(QStringLiteral("patrick"));

    const QJsonObject json = message.toJson();
    QCOMPARE(json.value(QStringLiteral("role")).toString(), QStringLiteral("user"));
    QCOMPARE(json.value(QStringLiteral("content")).toString(), QStringLiteral("Hello"));
    QCOMPARE(json.value(QStringLiteral("name")).toString(), QStringLiteral("patrick"));

    const Message parsed = Message::fromJson(json);
    QCOMPARE(parsed, message);
}

void TestSerialization::assistantToolCallSerialization()
{
    FunctionCall fn(QStringLiteral("get_weather"),
                    QStringLiteral("{\"location\":\"Berlin\"}"));
    ToolCall call(QStringLiteral("call_1"), fn);

    Message assistant;
    assistant.setRole(Role::Assistant);
    assistant.addToolCall(call);

    const QJsonObject json = assistant.toJson();
    // An assistant message with only tool calls must carry a null content.
    QVERIFY(json.value(QStringLiteral("content")).isNull());

    const QJsonArray toolCalls = json.value(QStringLiteral("tool_calls")).toArray();
    QCOMPARE(toolCalls.size(), 1);
    const QJsonObject callJson = toolCalls.first().toObject();
    QCOMPARE(callJson.value(QStringLiteral("id")).toString(), QStringLiteral("call_1"));
    QCOMPARE(callJson.value(QStringLiteral("type")).toString(), QStringLiteral("function"));

    const Message parsed = Message::fromJson(json);
    QCOMPARE(parsed.toolCalls().size(), 1);
    QCOMPARE(parsed.toolCalls().first(), call);
    QCOMPARE(parsed.toolCalls().first().function().argumentsObject()
                 .value(QStringLiteral("location")).toString(),
             QStringLiteral("Berlin"));
}

void TestSerialization::requestSerialization()
{
    ChatCompletionRequest request(QStringLiteral("gpt-4o"),
                                  {Message::system(QStringLiteral("You are helpful.")),
                                   Message::user(QStringLiteral("Weather in Berlin?"))});

    QJsonObject params{
        {QStringLiteral("type"), QStringLiteral("object")},
        {QStringLiteral("properties"),
         QJsonObject{{QStringLiteral("location"),
                      QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}}}},
    };
    request.addTool(Tool::function(QStringLiteral("get_weather"),
                                   QStringLiteral("Look up the weather"), params));
    request.setTemperature(0.2);
    request.setToolChoice(QStringLiteral("auto"));

    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("model")).toString(), QStringLiteral("gpt-4o"));
    QCOMPARE(json.value(QStringLiteral("messages")).toArray().size(), 2);
    QCOMPARE(json.value(QStringLiteral("tools")).toArray().size(), 1);
    QCOMPARE(json.value(QStringLiteral("temperature")).toDouble(), 0.2);
    QCOMPARE(json.value(QStringLiteral("tool_choice")).toString(), QStringLiteral("auto"));

    const ChatCompletionRequest parsed = ChatCompletionRequest::fromJson(json);
    QCOMPARE(parsed.model(), request.model());
    QCOMPARE(parsed.messages().size(), 2);
    QCOMPARE(parsed.tools().size(), 1);
    QCOMPARE(parsed.temperature(), std::optional<double>(0.2));
}

void TestSerialization::requestOmitsUnsetOptionals()
{
    ChatCompletionRequest request(QStringLiteral("gpt-4o"),
                                  {Message::user(QStringLiteral("hi"))});
    const QJsonObject json = request.toJson();
    QVERIFY(!json.contains(QStringLiteral("temperature")));
    QVERIFY(!json.contains(QStringLiteral("tools")));
    QVERIFY(!json.contains(QStringLiteral("stream")));
    QVERIFY(!json.contains(QStringLiteral("tool_choice")));
}

void TestSerialization::responseParsingFromSpecExample()
{
    const QByteArray body = R"({
        "id": "chatcmpl-abc",
        "object": "chat.completion",
        "created": 1741570283,
        "model": "gpt-4o-2024-08-06",
        "choices": [{
            "index": 0,
            "message": {"role": "assistant", "content": "Hello there."},
            "logprobs": null,
            "finish_reason": "stop"
        }],
        "usage": {"prompt_tokens": 10, "completion_tokens": 5, "total_tokens": 15}
    })";

    const ChatCompletionResponse response =
        ChatCompletionResponse::fromJson(QJsonDocument::fromJson(body).object());

    QCOMPARE(response.id(), QStringLiteral("chatcmpl-abc"));
    QCOMPARE(response.created(), Q_INT64_C(1741570283));
    QCOMPARE(response.choices().size(), 1);
    QCOMPARE(response.firstMessage().content(), QStringLiteral("Hello there."));
    QCOMPARE(response.choices().first().finishReason(), FinishReason::Stop);
    QCOMPARE(response.usage().totalTokens(), 15);
}

void TestSerialization::responseWithToolCalls()
{
    const QByteArray body = R"({
        "id": "chatcmpl-tools",
        "object": "chat.completion",
        "created": 1,
        "model": "gpt-4o",
        "choices": [{
            "index": 0,
            "message": {
                "role": "assistant",
                "content": null,
                "tool_calls": [{
                    "id": "call_42",
                    "type": "function",
                    "function": {"name": "get_weather", "arguments": "{\"location\":\"Paris\"}"}
                }]
            },
            "finish_reason": "tool_calls"
        }],
        "usage": {"prompt_tokens": 1, "completion_tokens": 1, "total_tokens": 2}
    })";

    const ChatCompletionResponse response =
        ChatCompletionResponse::fromJson(QJsonDocument::fromJson(body).object());

    QCOMPARE(response.choices().first().finishReason(), FinishReason::ToolCalls);
    const QList<ToolCall> calls = response.toolCalls();
    QCOMPARE(calls.size(), 1);
    QCOMPARE(calls.first().id(), QStringLiteral("call_42"));
    QCOMPARE(calls.first().function().name(), QStringLiteral("get_weather"));
    QCOMPARE(calls.first().function().argumentsObject().value(QStringLiteral("location")).toString(),
             QStringLiteral("Paris"));
}

QTEST_MAIN(TestSerialization)
#include "tst_serialization.moc"
