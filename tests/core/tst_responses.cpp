// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/Response.h>
#include <QtOpenAi/Core/ResponseRequest.h>

#include <QtCore/QJsonDocument>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// JSON round-trip and value-semantics coverage for the Responses-API types.
class TestResponses : public QObject
{
    Q_OBJECT
private slots:
    void requestRoundTrip();
    void requestOmitsUnsetOptionals();
    void parsesOutputItems();
    void responseRoundTrip();
    void outputConvenienceAccessors();
    void valueSemantics();
};

void TestResponses::requestRoundTrip()
{
    ResponseRequest request(QStringLiteral("gpt-5"), QStringLiteral("Tell me a joke"));
    request.setInstructions(QStringLiteral("Be concise"));
    request.setMaxOutputTokens(256);
    request.setTemperature(0.7);
    request.setTopP(0.9);
    request.setStore(true);
    request.setPreviousResponseId(QStringLiteral("resp_prev"));
    request.setReasoningEffort(QStringLiteral("high"));
    request.setToolChoice(QStringLiteral("auto"));
    request.addTool(Tool::function(QStringLiteral("get_weather"), QStringLiteral("Get the weather"),
                                   QJsonObject {}));
    request.setMetadata(QJsonObject {{QStringLiteral("trace"), QStringLiteral("abc")}});

    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("model")).toString(), QStringLiteral("gpt-5"));
    QCOMPARE(json.value(QStringLiteral("input")).toString(), QStringLiteral("Tell me a joke"));
    QCOMPARE(json.value(QStringLiteral("max_output_tokens")).toInt(), 256);
    QCOMPARE(json.value(QStringLiteral("reasoning"))
                     .toObject()
                     .value(QStringLiteral("effort"))
                     .toString(),
             QStringLiteral("high"));
    QCOMPARE(json.value(QStringLiteral("tools")).toArray().size(), 1);

    const ResponseRequest parsed = ResponseRequest::fromJson(json);
    QCOMPARE(parsed, request);
}

void TestResponses::requestOmitsUnsetOptionals()
{
    const ResponseRequest request(QStringLiteral("gpt-5"), QStringLiteral("hi"));
    const QJsonObject json = request.toJson();

    QVERIFY(!json.contains(QStringLiteral("temperature")));
    QVERIFY(!json.contains(QStringLiteral("max_output_tokens")));
    QVERIFY(!json.contains(QStringLiteral("store")));
    QVERIFY(!json.contains(QStringLiteral("reasoning")));
    QVERIFY(!json.contains(QStringLiteral("tools")));
    QVERIFY(!json.contains(QStringLiteral("stream")));
}

void TestResponses::parsesOutputItems()
{
    const QByteArray body = R"({
        "id": "resp_123", "object": "response", "created_at": 1700000000,
        "model": "gpt-5", "status": "completed",
        "output": [
            {"type": "reasoning", "id": "rs_1",
             "summary": [{"type": "summary_text", "text": "thinking"}]},
            {"type": "function_call", "id": "fc_1", "call_id": "call_1",
             "name": "get_weather", "arguments": "{\"city\":\"Berlin\"}", "status": "completed"},
            {"type": "message", "id": "msg_1", "status": "completed", "role": "assistant",
             "content": [{"type": "output_text", "text": "Hello!", "annotations": []}]}
        ],
        "usage": {"input_tokens": 10, "output_tokens": 7, "total_tokens": 17,
                  "output_tokens_details": {"reasoning_tokens": 4}}
    })";
    const Response response = Response::fromJson(QJsonDocument::fromJson(body).object());

    QCOMPARE(response.id(), QStringLiteral("resp_123"));
    QCOMPARE(response.status(), QStringLiteral("completed"));
    QCOMPARE(response.output().size(), 3);

    const ResponseOutputItem reasoning = response.output().at(0);
    QVERIFY(reasoning.isReasoning());
    QCOMPARE(reasoning.summary(), QStringList {QStringLiteral("thinking")});

    const ResponseOutputItem call = response.output().at(1);
    QVERIFY(call.isFunctionCall());
    QCOMPARE(call.name(), QStringLiteral("get_weather"));
    QCOMPARE(call.arguments(), QStringLiteral("{\"city\":\"Berlin\"}"));
    QCOMPARE(call.callId(), QStringLiteral("call_1"));

    const ResponseOutputItem message = response.output().at(2);
    QVERIFY(message.isMessage());
    QCOMPARE(message.role(), QStringLiteral("assistant"));
    QCOMPARE(message.text(), QStringLiteral("Hello!"));

    QCOMPARE(response.usage().inputTokens(), 10);
    QCOMPARE(response.usage().outputTokens(), 7);
    QCOMPARE(response.usage().reasoningTokens(), 4);
}

void TestResponses::responseRoundTrip()
{
    Response response;
    response.setId(QStringLiteral("resp_abc"));
    response.setCreatedAt(1700000000);
    response.setModel(QStringLiteral("gpt-5"));
    response.setStatus(QStringLiteral("completed"));
    response.addOutput(ResponseOutputItem::reasoning({QStringLiteral("step one")}));
    response.addOutput(ResponseOutputItem::functionCall(
            QStringLiteral("lookup"), QStringLiteral("{}"), QStringLiteral("call_9")));
    response.addOutput(ResponseOutputItem::message(QStringLiteral("Done.")));

    ResponseUsage usage;
    usage.setInputTokens(5);
    usage.setOutputTokens(3);
    usage.setTotalTokens(8);
    usage.setReasoningTokens(1);
    response.setUsage(usage);

    const Response parsed = Response::fromJson(response.toJson());
    QCOMPARE(parsed, response);
}

void TestResponses::outputConvenienceAccessors()
{
    Response response;
    response.addOutput(ResponseOutputItem::message(QStringLiteral("Hello ")));
    response.addOutput(ResponseOutputItem::functionCall(QStringLiteral("f"), QStringLiteral("{}"),
                                                        QStringLiteral("call_1")));
    response.addOutput(ResponseOutputItem::message(QStringLiteral("world")));

    QCOMPARE(response.outputText(), QStringLiteral("Hello world"));
    QCOMPARE(response.functionCalls().size(), 1);
    QCOMPARE(response.functionCalls().first().name(), QStringLiteral("f"));
}

void TestResponses::valueSemantics()
{
    Response a;
    a.setId(QStringLiteral("resp_1"));
    Response b = a;
    QCOMPARE(a, b);
    b.setId(QStringLiteral("resp_2"));
    QVERIFY(a != b);
    QCOMPARE(a.id(), QStringLiteral("resp_1"));
}

QTEST_APPLESS_MAIN(TestResponses)
#include "tst_responses.moc"
