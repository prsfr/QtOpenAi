// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/Batch.h>
#include <QtOpenAi/Core/ChatCompletionChunk.h>
#include <QtOpenAi/Core/ChatCompletionResponse.h>
#include <QtOpenAi/Core/CompletionResponse.h>
#include <QtOpenAi/Core/EmbeddingResponse.h>
#include <QtOpenAi/Core/EvalRun.h>
#include <QtOpenAi/Core/FineTuningCheckpoint.h>
#include <QtOpenAi/Core/FineTuningJob.h>
#include <QtOpenAi/Core/ImageResponse.h>
#include <QtOpenAi/Core/Message.h>
#include <QtOpenAi/Core/ModerationResponse.h>
#include <QtOpenAi/Core/Response.h>
#include <QtOpenAi/Core/ResponseOutputItem.h>
#include <QtOpenAi/Core/Tool.h>
#include <QtOpenAi/Core/ToolCall.h>
#include <QtOpenAi/Core/TranscriptionResponse.h>
#include <QtOpenAi/Core/Usage.h>
#include <QtOpenAi/Core/VectorStore.h>
#include <QtOpenAi/Core/VectorStoreSearch.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Decode/encode symmetry for the value types that only ever had decode tested.
//
// Half the serialisable types were exercised in one direction only: a wire
// payload was parsed and a few getters asserted, so a field missing from
// toJson() -- present in the type, readable, simply never written back -- was
// invisible. Round-tripping the *JSON* rather than the object catches that
// without needing to know each type's setter API: every key the server sent has
// to come back out.
//
// The encoder may add keys (it fills in defaults the server omitted); it may not
// lose or change one. Where a type drops a field on purpose the reason is named
// at the call site.
class TestRoundTrip : public QObject
{
    Q_OBJECT
private slots:
    void chatCompletionResponseKeepsEveryField();
    void chatCompletionChunkKeepsEveryField();
    void completionResponseKeepsEveryField();
    void toolAndToolCallKeepEveryField();
    void messageAndContentPartsKeepEveryField();
    void responseOutputItemKeepsEveryField();
    void embeddingResponseKeepsEveryField();
    void imageResponseKeepsEveryField();
    void moderationResponseKeepsEveryField();
    void transcriptionResponseKeepsEveryField();
    void plainAggregatesKeepEveryField();
    void searchResultKeepsEveryField();
};

namespace {

QString path(const QString &prefix, const QString &key)
{
    return prefix.isEmpty() ? key : prefix + QLatin1Char('.') + key;
}

// Every key in `wire` must be present in `encoded` with an equal value.
// Recurses into nested objects and arrays so a field buried in a choice or a
// content part is checked too.
void assertKeeps(const QJsonValue &wire, const QJsonValue &encoded, const QString &where,
                 const QStringList &ignored)
{
    if (wire.isObject()) {
        QVERIFY2(encoded.isObject(),
                 qPrintable(QStringLiteral("%1: expected an object").arg(where)));
        const QJsonObject wireObject = wire.toObject();
        const QJsonObject encodedObject = encoded.toObject();
        for (auto it = wireObject.constBegin(); it != wireObject.constEnd(); ++it) {
            const QString at = path(where, it.key());
            if (ignored.contains(at))
                continue;
            QVERIFY2(encodedObject.contains(it.key()),
                     qPrintable(QStringLiteral("%1 was dropped by toJson()").arg(at)));
            assertKeeps(it.value(), encodedObject.value(it.key()), at, ignored);
        }
        return;
    }
    if (wire.isArray()) {
        QVERIFY2(encoded.isArray(), qPrintable(QStringLiteral("%1: expected an array").arg(where)));
        const QJsonArray wireArray = wire.toArray();
        const QJsonArray encodedArray = encoded.toArray();
        QCOMPARE(encodedArray.size(), wireArray.size());
        for (int i = 0; i < wireArray.size(); ++i)
            assertKeeps(wireArray.at(i), encodedArray.at(i),
                        QStringLiteral("%1[%2]").arg(where).arg(i), ignored);
        return;
    }
    QVERIFY2(encoded == wire,
             qPrintable(QStringLiteral("%1 changed: %2 -> %3")
                                .arg(where,
                                     QString::fromUtf8(QJsonDocument(QJsonArray {wire})
                                                               .toJson(QJsonDocument::Compact)),
                                     QString::fromUtf8(QJsonDocument(QJsonArray {encoded})
                                                               .toJson(QJsonDocument::Compact)))));
}

QJsonObject parse(const char *json)
{
    QJsonParseError error {};
    const QJsonDocument doc = QJsonDocument::fromJson(QByteArray(json), &error);
    Q_ASSERT(error.error == QJsonParseError::NoError);
    return doc.object();
}

// Decode `json` into T, encode it again, and require every key to survive.
template <typename T>
void checkKeepsFields(const char *json, const QStringList &ignored = {})
{
    const QJsonObject wire = parse(json);
    assertKeeps(wire, T::fromJson(wire).toJson(), QString(), ignored);
}

} // namespace

void TestRoundTrip::chatCompletionResponseKeepsEveryField()
{
    checkKeepsFields<ChatCompletionResponse>(R"({
        "id": "chatcmpl-1", "object": "chat.completion", "created": 1700000000,
        "model": "gpt-4o", "system_fingerprint": "fp_1",
        "choices": [{
            "index": 0,
            "message": {"role": "assistant", "content": "hi"},
            "finish_reason": "stop"
        }],
        "usage": {"prompt_tokens": 1, "completion_tokens": 2, "total_tokens": 3}
    })");
}

void TestRoundTrip::chatCompletionChunkKeepsEveryField()
{
    checkKeepsFields<ChatCompletionChunk>(R"({
        "id": "chatcmpl-1", "object": "chat.completion.chunk", "created": 1700000000,
        "model": "gpt-4o",
        "choices": [{
            "index": 0,
            "delta": {"role": "assistant", "content": "hi"},
            "finish_reason": "stop"
        }]
    })");
}

void TestRoundTrip::completionResponseKeepsEveryField()
{
    checkKeepsFields<CompletionResponse>(R"({
        "id": "cmpl-1", "object": "text_completion", "created": 1700000000,
        "model": "gpt-3.5-turbo-instruct",
        "choices": [{"index": 0, "text": "hello", "finish_reason": "length"}],
        "usage": {"prompt_tokens": 1, "completion_tokens": 2, "total_tokens": 3}
    })");
}

void TestRoundTrip::toolAndToolCallKeepEveryField()
{
    checkKeepsFields<Tool>(R"({
        "type": "function",
        "function": {
            "name": "get_weather",
            "description": "Look up the weather",
            "parameters": {"type": "object", "properties": {"city": {"type": "string"}}}
        }
    })");

    checkKeepsFields<ToolCall>(R"({
        "id": "call_1", "type": "function",
        "function": {"name": "get_weather", "arguments": "{\"city\":\"Berlin\"}"}
    })");
}

void TestRoundTrip::messageAndContentPartsKeepEveryField()
{
    checkKeepsFields<Message>(R"({
        "role": "assistant", "name": "assistant-1", "content": "hello",
        "tool_calls": [{
            "id": "call_1", "type": "function",
            "function": {"name": "f", "arguments": "{}"}
        }]
    })");

    // Multimodal content: the parts array must survive intact.
    checkKeepsFields<Message>(R"({
        "role": "user",
        "content": [
            {"type": "text", "text": "what is this?"},
            {"type": "image_url", "image_url": {"url": "https://example.invalid/a.png"}}
        ]
    })");
}

void TestRoundTrip::responseOutputItemKeepsEveryField()
{
    checkKeepsFields<ResponseOutputItem>(R"({
        "id": "msg_1", "type": "message", "role": "assistant", "status": "completed",
        "content": [{"type": "output_text", "text": "hello"}]
    })");

    checkKeepsFields<ResponseOutputItem>(R"({
        "id": "fc_1", "type": "function_call", "status": "completed",
        "name": "get_weather", "arguments": "{}", "call_id": "call_1"
    })");
}

void TestRoundTrip::embeddingResponseKeepsEveryField()
{
    checkKeepsFields<EmbeddingResponse>(R"({
        "object": "list", "model": "text-embedding-3-small",
        "data": [{"object": "embedding", "index": 0, "embedding": [0.5, -0.25]}],
        "usage": {"prompt_tokens": 4, "total_tokens": 4}
    })");
}

void TestRoundTrip::imageResponseKeepsEveryField()
{
    checkKeepsFields<ImageResponse>(R"({
        "created": 1700000000,
        "data": [{"b64_json": "aGk=", "revised_prompt": "a cat, refined"}]
    })");
}

void TestRoundTrip::moderationResponseKeepsEveryField()
{
    checkKeepsFields<ModerationResponse>(R"({
        "id": "modr-1", "model": "omni-moderation-latest",
        "results": [{
            "flagged": true,
            "categories": {"violence": true, "hate": false},
            "category_scores": {"violence": 0.9, "hate": 0.01}
        }]
    })");
}

void TestRoundTrip::transcriptionResponseKeepsEveryField()
{
    checkKeepsFields<TranscriptionResponse>(R"({
        "task": "transcribe", "language": "english", "duration": 1.5,
        "text": "hello there",
        "segments": [{
            "id": 0, "seek": 0, "start": 0.0, "end": 1.5, "text": "hello there",
            "temperature": 0.0, "avg_logprob": -0.3, "compression_ratio": 1.1,
            "no_speech_prob": 0.01
        }],
        "words": [{"word": "hello", "start": 0.0, "end": 0.5}]
    })");
}

void TestRoundTrip::plainAggregatesKeepEveryField()
{
    checkKeepsFields<Usage>(R"({"prompt_tokens": 1, "completion_tokens": 2, "total_tokens": 3})");
    checkKeepsFields<BatchRequestCounts>(R"({"total": 3, "completed": 2, "failed": 1})");
    checkKeepsFields<BatchError>(
            R"({"code": "invalid_json_line", "message": "bad", "param": "body", "line": 7})");
    checkKeepsFields<EvalResultCounts>(R"({"total": 4, "errored": 1, "failed": 1, "passed": 2})");
    checkKeepsFields<VectorStoreFileCounts>(R"({"in_progress": 1, "completed": 2,
                                                "cancelled": 0, "failed": 0, "total": 3})");
    checkKeepsFields<FineTuningCheckpointMetrics>(R"({
        "step": 88.0, "train_loss": 0.4, "train_mean_token_accuracy": 0.9,
        "valid_loss": 0.5, "valid_mean_token_accuracy": 0.8,
        "full_valid_loss": 0.6, "full_valid_mean_token_accuracy": 0.7
    })");
}

void TestRoundTrip::searchResultKeepsEveryField()
{
    checkKeepsFields<VectorStoreSearchResult>(R"({
        "file_id": "file-1", "filename": "notes.txt", "score": 0.87,
        "attributes": {"team": "qa"},
        "content": [{"type": "text", "text": "the answer"}]
    })");
}

QTEST_MAIN(TestRoundTrip)
#include "tst_roundtrip.moc"
