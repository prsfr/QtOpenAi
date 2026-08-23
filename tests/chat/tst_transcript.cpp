// SPDX-License-Identifier: MIT
#include <QtOpenAi/Chat/Transcript.h>

#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Chat;

namespace {

QStringList contentsOf(const QList<Message> &messages)
{
    QStringList contents;
    for (const Message &message : messages)
        contents.append(message.content());
    return contents;
}

} // namespace

// Coverage for client-side conversation history (#35). The type is a tree; the
// linear case is the tree that never branched, and both are pinned here.
class TestTranscript : public QObject
{
    Q_OBJECT
private slots:
    void appendsIntoALinearConversation();
    void putsTheSystemPromptInFront();
    void forkCreatesASiblingRatherThanOverwriting();
    void switchingTheActiveLeafChangesTheContext();
    void reportsTheTreeAroundANode();
    void rejectsOperationsOnNodesItDoesNotHave();
    void aDanglingParentEndsTheWalkWhereItAlwaysDid();
    void roundTripsThroughJson();
    void buildsARequestFromTheActivePath();
};

void TestTranscript::appendsIntoALinearConversation()
{
    Transcript transcript;
    const Transcript::NodeId first = transcript.addUserMessage(QStringLiteral("one"));
    const Transcript::NodeId second
            = transcript.addMessage(Message(Role::Assistant, QStringLiteral("two")));

    QCOMPARE(transcript.count(), 2);
    QCOMPARE(transcript.activeLeaf(), second);
    QCOMPARE(transcript.parent(second), first);
    QCOMPARE(transcript.parent(first), Transcript::InvalidNode);
    QCOMPARE(transcript.activePath(), QList<Transcript::NodeId>({first, second}));
    QCOMPARE(contentsOf(transcript.messages()),
             QStringList({QStringLiteral("one"), QStringLiteral("two")}));
}

void TestTranscript::putsTheSystemPromptInFront()
{
    // It belongs to the conversation rather than to a turn in it, so it lives
    // outside the tree and leads every context built from it.
    Transcript transcript;
    transcript.setSystemPrompt(QStringLiteral("be terse"));
    transcript.addUserMessage(QStringLiteral("hello"));

    const QList<Message> messages = transcript.messages();
    QCOMPARE(messages.size(), 2);
    QCOMPARE(messages.first().role(), Role::System);
    QCOMPARE(messages.first().content(), QStringLiteral("be terse"));
    // ... and it is not a node, so it is not part of the tree.
    QCOMPARE(transcript.count(), 1);
}

void TestTranscript::forkCreatesASiblingRatherThanOverwriting()
{
    // Editing a past question is what every chat UI's "‹ 2/2 ›" control does:
    // the old answer stays reachable.
    Transcript transcript;
    const Transcript::NodeId question = transcript.addUserMessage(QStringLiteral("first ask"));
    transcript.addMessage(Message(Role::Assistant, QStringLiteral("first answer")));

    const Transcript::NodeId edited
            = transcript.fork(question, Message::user(QStringLiteral("second ask")));

    QCOMPARE(transcript.parent(edited), transcript.parent(question));
    QCOMPARE(transcript.activeLeaf(), edited);
    QCOMPARE(transcript.siblings(question), QList<Transcript::NodeId>({question, edited}));
    // The new branch is the context; the old one is history, not lost.
    QCOMPARE(contentsOf(transcript.messages()), QStringList({QStringLiteral("second ask")}));
    QCOMPARE(transcript.count(), 3);
}

void TestTranscript::switchingTheActiveLeafChangesTheContext()
{
    Transcript transcript;
    transcript.addUserMessage(QStringLiteral("ask"));
    const Transcript::NodeId original
            = transcript.addMessage(Message(Role::Assistant, QStringLiteral("answer A")));
    const Transcript::NodeId regenerated
            = transcript.fork(original, Message(Role::Assistant, QStringLiteral("answer B")));

    QCOMPARE(contentsOf(transcript.messages()),
             QStringList({QStringLiteral("ask"), QStringLiteral("answer B")}));

    QVERIFY(transcript.setActiveLeaf(original));
    QCOMPARE(contentsOf(transcript.messages()),
             QStringList({QStringLiteral("ask"), QStringLiteral("answer A")}));
    QCOMPARE(transcript.activeLeaf(), original);
    Q_UNUSED(regenerated)
}

void TestTranscript::reportsTheTreeAroundANode()
{
    Transcript transcript;
    const Transcript::NodeId root = transcript.addUserMessage(QStringLiteral("root"));
    const Transcript::NodeId child = transcript.addUserMessage(QStringLiteral("child"));
    const Transcript::NodeId other = transcript.fork(child, Message::user(QStringLiteral("other")));

    QCOMPARE(transcript.roots(), QList<Transcript::NodeId>({root}));
    QCOMPARE(transcript.children(root), QList<Transcript::NodeId>({child, other}));
    QCOMPARE(transcript.children(other), QList<Transcript::NodeId>());
    QCOMPARE(transcript.message(child).content(), QStringLiteral("child"));
    QVERIFY(transcript.contains(child));
}

void TestTranscript::rejectsOperationsOnNodesItDoesNotHave()
{
    Transcript transcript;
    transcript.addUserMessage(QStringLiteral("only"));

    const Transcript::NodeId ghost = 999;
    QVERIFY(!transcript.contains(ghost));
    QCOMPARE(transcript.fork(ghost, Message::user(QStringLiteral("x"))), Transcript::InvalidNode);
    QVERIFY(!transcript.setActiveLeaf(ghost));
    QVERIFY(!transcript.setMessage(ghost, Message::user(QStringLiteral("x"))));
    QVERIFY(transcript.message(ghost).content().isEmpty());
    QVERIFY(transcript.siblings(ghost).isEmpty());
    // The failed calls left the transcript as it was.
    QCOMPARE(transcript.count(), 1);
}

void TestTranscript::aDanglingParentEndsTheWalkWhereItAlwaysDid()
{
    // fromJson() keeps a node's parent id even when no such node was written,
    // treating it as a root. The active path then runs off the end of the tree,
    // and messages() has always answered with an empty message where the
    // missing node would have been. activePath() and messages() are now one
    // walk rather than two, so this pins that they still agree, and still
    // agree with what reading a missing node used to give.
    const QJsonObject json {
            {QStringLiteral("nodes"),
             QJsonArray {QJsonObject {{QStringLiteral("id"), 1},
                                      {QStringLiteral("parent"), 99}, // never written
                                      {QStringLiteral("message"),
                                       Message::user(QStringLiteral("orphan")).toJson()}}}},
            {QStringLiteral("active_leaf"), 1}};

    const Transcript transcript = Transcript::fromJson(json);
    QCOMPARE(transcript.activeLeaf(), 1);
    QVERIFY(!transcript.contains(99));

    // Root-first, the missing id included -- the path is what it was.
    QCOMPARE(transcript.activePath(), QList<Transcript::NodeId>({99, 1}));
    QCOMPARE(transcript.parent(1), 99);
    QCOMPARE(transcript.parent(99), Transcript::InvalidNode);

    // One message per id on the path, in the same order, the missing one empty.
    const QList<Message> messages = transcript.messages();
    QCOMPARE(messages.size(), transcript.activePath().size());
    QVERIFY(messages.first().content().isEmpty());
    QCOMPARE(messages.last().content(), QStringLiteral("orphan"));
}

void TestTranscript::roundTripsThroughJson()
{
    Transcript transcript;
    transcript.setSystemPrompt(QStringLiteral("be terse"));
    const Transcript::NodeId question = transcript.addUserMessage(QStringLiteral("ask"));
    transcript.addMessage(Message(Role::Assistant, QStringLiteral("answer A")));
    transcript.fork(question, Message::user(QStringLiteral("ask again")));
    transcript.addMessage(Message(Role::Assistant, QStringLiteral("answer B")));

    Transcript restored = Transcript::fromJson(transcript.toJson());

    // Node ids survive, so anything the application stored alongside them still
    // points at the right message.
    QCOMPARE(restored, transcript);
    QCOMPARE(restored.activeLeaf(), transcript.activeLeaf());
    QCOMPARE(restored.children(question), transcript.children(question));
    QCOMPARE(contentsOf(restored.messages()), contentsOf(transcript.messages()));

    // A branch that was not active is still there to switch back to.
    QVERIFY(restored.setActiveLeaf(question));
    QCOMPARE(contentsOf(restored.messages()),
             QStringList({QStringLiteral("be terse"), QStringLiteral("ask")}));
}

void TestTranscript::buildsARequestFromTheActivePath()
{
    Transcript transcript;
    transcript.setSystemPrompt(QStringLiteral("be terse"));
    transcript.addUserMessage(QStringLiteral("hello"));

    const ChatCompletionRequest request = transcript.buildRequest(QStringLiteral("gpt-4o-mini"));

    QCOMPARE(request.model(), QStringLiteral("gpt-4o-mini"));
    QCOMPARE(contentsOf(request.messages()),
             QStringList({QStringLiteral("be terse"), QStringLiteral("hello")}));
}

QTEST_MAIN(TestTranscript)
#include "tst_transcript.moc"
