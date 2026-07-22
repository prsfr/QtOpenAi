// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/Message.h>
#include <QtOpenAi/Core/ToolCall.h>

#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Verifies the implicitly-shared (copy-on-write) value semantics of the Core
// data types, i.e. that the QSharedDataPointer d-pointer detaches correctly.
class TestValueSemantics : public QObject
{
    Q_OBJECT
private slots:
    void copyIsIndependentAfterMutation();
    void equalityAndInequality();
    void moveLeavesUsableState();
    void toolResultFactory();
};

void TestValueSemantics::copyIsIndependentAfterMutation()
{
    Message original = Message::user(QStringLiteral("original"));
    Message copy = original;

    QCOMPARE(copy.content(), QStringLiteral("original"));

    // Mutating the copy must not affect the original (copy-on-write detach).
    copy.setContent(QStringLiteral("changed"));
    QCOMPARE(copy.content(), QStringLiteral("changed"));
    QCOMPARE(original.content(), QStringLiteral("original"));
}

void TestValueSemantics::equalityAndInequality()
{
    Message a = Message::assistant(QStringLiteral("hi"));
    Message b = Message::assistant(QStringLiteral("hi"));
    Message c = Message::assistant(QStringLiteral("bye"));

    QVERIFY(a == b);
    QVERIFY(a != c);

    ToolCall t1(QStringLiteral("id"), FunctionCall(QStringLiteral("f"), QStringLiteral("{}")));
    ToolCall t2(QStringLiteral("id"), FunctionCall(QStringLiteral("f"), QStringLiteral("{}")));
    QVERIFY(t1 == t2);
}

void TestValueSemantics::moveLeavesUsableState()
{
    Message source = Message::user(QStringLiteral("payload"));
    Message moved = std::move(source);
    QCOMPARE(moved.content(), QStringLiteral("payload"));

    // A moved-from message must remain safely usable (Qt value-type contract).
    source = Message::user(QStringLiteral("reused"));
    QCOMPARE(source.content(), QStringLiteral("reused"));
}

void TestValueSemantics::toolResultFactory()
{
    Message result = Message::toolResult(QStringLiteral("call_7"), QStringLiteral("42"));
    QCOMPARE(result.role(), Role::Tool);
    QCOMPARE(result.toolCallId(), QStringLiteral("call_7"));
    QCOMPARE(result.content(), QStringLiteral("42"));

    const QJsonObject json = result.toJson();
    QCOMPARE(json.value(QStringLiteral("role")).toString(), QStringLiteral("tool"));
    QCOMPARE(json.value(QStringLiteral("tool_call_id")).toString(), QStringLiteral("call_7"));
}

QTEST_MAIN(TestValueSemantics)
#include "tst_valuesemantics.moc"
