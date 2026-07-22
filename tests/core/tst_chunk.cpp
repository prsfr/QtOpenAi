// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/ChatCompletionChunk.h>

#include <QtCore/QJsonDocument>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Verifies parsing of streamed `chat.completion.chunk` objects.
class TestChunk : public QObject
{
    Q_OBJECT
private slots:
    void parsesContentDelta();
    void parsesRoleAndFinishReason();
    void parsesToolCallFragment();
    void roundTrip();
};

void TestChunk::parsesContentDelta()
{
    const QByteArray body = R"({
        "id": "chatcmpl-x", "object": "chat.completion.chunk", "created": 5, "model": "gpt-4o",
        "choices": [{"index": 0, "delta": {"content": "Hel"}, "finish_reason": null}]
    })";
    const ChatCompletionChunk chunk
            = ChatCompletionChunk::fromJson(QJsonDocument::fromJson(body).object());

    QCOMPARE(chunk.id(), QStringLiteral("chatcmpl-x"));
    QCOMPARE(chunk.model(), QStringLiteral("gpt-4o"));
    QCOMPARE(chunk.choices().size(), 1);
    const ChoiceDelta delta = chunk.choices().first().delta();
    QVERIFY(delta.hasContent());
    QCOMPARE(delta.content(), QStringLiteral("Hel"));
    QVERIFY(!delta.role().has_value());
    QCOMPARE(chunk.choices().first().finishReason(), FinishReason::None);
}

void TestChunk::parsesRoleAndFinishReason()
{
    const QByteArray body = R"({
        "choices": [{"index": 0, "delta": {"role": "assistant", "content": ""},
                     "finish_reason": "stop"}]
    })";
    const ChatCompletionChunk chunk
            = ChatCompletionChunk::fromJson(QJsonDocument::fromJson(body).object());

    const ChunkChoice choice = chunk.choices().first();
    QCOMPARE(choice.delta().role(), std::optional<Role>(Role::Assistant));
    QCOMPARE(choice.finishReason(), FinishReason::Stop);
}

void TestChunk::parsesToolCallFragment()
{
    const QByteArray body = R"({
        "choices": [{"index": 0, "delta": {"tool_calls": [
            {"index": 0, "id": "call_1", "type": "function",
             "function": {"name": "get_weather", "arguments": "{\"loc"}}
        ]}, "finish_reason": null}]
    })";
    const ChatCompletionChunk chunk
            = ChatCompletionChunk::fromJson(QJsonDocument::fromJson(body).object());

    const QList<ToolCallChunk> calls = chunk.choices().first().delta().toolCalls();
    QCOMPARE(calls.size(), 1);
    QCOMPARE(calls.first().index(), 0);
    QCOMPARE(calls.first().id(), QStringLiteral("call_1"));
    QCOMPARE(calls.first().functionName(), QStringLiteral("get_weather"));
    QCOMPARE(calls.first().argumentsFragment(), QStringLiteral("{\"loc"));
}

void TestChunk::roundTrip()
{
    ChatCompletionChunk chunk;
    chunk.setId(QStringLiteral("id"));
    chunk.setModel(QStringLiteral("m"));

    ChoiceDelta delta;
    delta.setRole(Role::Assistant);
    delta.setContent(QStringLiteral("hi"));

    ChunkChoice choice;
    choice.setIndex(0);
    choice.setDelta(delta);
    choice.setFinishReason(FinishReason::Stop);
    chunk.setChoices({choice});

    const ChatCompletionChunk parsed = ChatCompletionChunk::fromJson(chunk.toJson());
    QCOMPARE(parsed, chunk);
}

QTEST_MAIN(TestChunk)
#include "tst_chunk.moc"
