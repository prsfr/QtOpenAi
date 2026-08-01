// SPDX-License-Identifier: MIT
#include <QtOpenAi/Chat/ConversationModel.h>

#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Chat;

// Coverage for the QObject wrapper (#35). The value type cannot announce
// anything; this is the part a view connects to, so what it emits and when is
// the contract.
class TestConversationModel : public QObject
{
    Q_OBJECT
private slots:
    void announcesEachAddedMessage();
    void announcesABranchSwitch();
    void reportsNoChangeAsNoSignal();
    void replacingTheTranscriptResets();
    void keepsTheTrimPolicyAcrossAReplacement();
};

void TestConversationModel::announcesEachAddedMessage()
{
    ConversationModel model;
    QSignalSpy addedSpy(&model, &ConversationModel::messageAdded);
    QSignalSpy countSpy(&model, &ConversationModel::countChanged);

    const int first = model.addUserMessage(QStringLiteral("one"));
    const int second = model.addMessage(Message(Role::Assistant, QStringLiteral("two")));

    QCOMPARE(addedSpy.count(), 2);
    // The parent comes with it, so a tree view knows where to put the row.
    QCOMPARE(addedSpy.at(0).at(0).toInt(), first);
    QCOMPARE(addedSpy.at(0).at(1).toInt(), Transcript::InvalidNode);
    QCOMPARE(addedSpy.at(1).at(1).toInt(), first);
    QCOMPARE(countSpy.count(), 2);
    QCOMPARE(model.count(), 2);
    QCOMPARE(model.activeLeaf(), second);
}

void TestConversationModel::announcesABranchSwitch()
{
    ConversationModel model;
    const int question = model.addUserMessage(QStringLiteral("ask"));
    const int answer = model.addMessage(Message(Role::Assistant, QStringLiteral("answer A")));

    QSignalSpy branchSpy(&model, &ConversationModel::activeBranchChanged);

    // A fork changes the whole path below it, not just its end -- a view has to
    // redraw rather than append.
    const int regenerated
            = model.fork(answer, Message(Role::Assistant, QStringLiteral("answer B")));
    QCOMPARE(branchSpy.count(), 1);
    QCOMPARE(branchSpy.first().first().toInt(), regenerated);
    QCOMPARE(model.siblings(answer), QList<Transcript::NodeId>({answer, regenerated}));

    QVERIFY(model.setActiveLeaf(answer));
    QCOMPARE(branchSpy.count(), 2);
    QCOMPARE(model.activePath(), QList<Transcript::NodeId>({question, answer}));
}

void TestConversationModel::reportsNoChangeAsNoSignal()
{
    ConversationModel model;
    model.setSystemPrompt(QStringLiteral("be terse"));

    QSignalSpy promptSpy(&model, &ConversationModel::systemPromptChanged);
    QSignalSpy branchSpy(&model, &ConversationModel::activeBranchChanged);
    QSignalSpy resetSpy(&model, &ConversationModel::reset);

    model.setSystemPrompt(QStringLiteral("be terse")); // same value
    QVERIFY(model.setActiveLeaf(model.activeLeaf()));  // same leaf
    model.clear();                                     // already empty

    QCOMPARE(promptSpy.count(), 0);
    QCOMPARE(branchSpy.count(), 0);
    QCOMPARE(resetSpy.count(), 0);

    // A node it does not have is refused rather than announced.
    QVERIFY(!model.setActiveLeaf(999));
    QCOMPARE(branchSpy.count(), 0);
    QCOMPARE(model.fork(999, Message::user(QStringLiteral("x"))), Transcript::InvalidNode);
}

void TestConversationModel::replacingTheTranscriptResets()
{
    Transcript loaded;
    loaded.setSystemPrompt(QStringLiteral("restored"));
    loaded.addUserMessage(QStringLiteral("from disk"));

    ConversationModel model;
    QSignalSpy resetSpy(&model, &ConversationModel::reset);
    QSignalSpy promptSpy(&model, &ConversationModel::systemPromptChanged);

    model.setTranscript(loaded);

    // Nothing incremental survives a wholesale replacement, so a view is told
    // to start over.
    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(promptSpy.count(), 1);
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.messages().size(), 2);
    // The transcript is a value: what comes back out is a copy.
    Transcript copy = model.transcript();
    copy.addUserMessage(QStringLiteral("not in the model"));
    QCOMPARE(model.count(), 1);
}

void TestConversationModel::keepsTheTrimPolicyAcrossAReplacement()
{
    // The policy is this model's configuration, not part of the history being
    // loaded into it.
    ConversationModel model;
    TrimPolicy policy;
    policy.setMaxMessages(2);
    model.setTrimPolicy(policy);

    Transcript loaded;
    for (int i = 1; i <= 5; ++i)
        loaded.addUserMessage(QStringLiteral("q%1").arg(i));
    model.setTranscript(loaded);

    QCOMPARE(model.trimPolicy().maxMessages(), 2);
    QCOMPARE(model.messages().size(), 2);
    QCOMPARE(model.count(), 5);
}

QTEST_MAIN(TestConversationModel)
#include "tst_conversationmodel.moc"
