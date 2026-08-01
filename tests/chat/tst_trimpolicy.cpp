// SPDX-License-Identifier: MIT
#include <QtOpenAi/Chat/Transcript.h>
#include <QtOpenAi/Chat/TrimPolicy.h>

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

QList<Message> conversation(int turns)
{
    QList<Message> messages;
    for (int i = 1; i <= turns; ++i) {
        messages.append(Message::user(QStringLiteral("q%1").arg(i)));
        messages.append(Message(Role::Assistant, QStringLiteral("a%1").arg(i)));
    }
    return messages;
}

} // namespace

// Coverage for context-window trimming (#35). What is dropped is the
// application's decision; what must never be dropped is this library's, and
// that is what these pin.
class TestTrimPolicy : public QObject
{
    Q_OBJECT
private slots:
    void keepsEverythingWithoutLimits();
    void dropsTheOldestTurnsFirst();
    void alwaysKeepsTheSystemPrompt();
    void neverLetsAToolResultLead();
    void keepsTheNewestTurnEvenWhenItDoesNotFit();
    void countsAgainstATokenBudget();
    void reservesRoomForTheReply();
    void handsTheDroppedMessagesToASummariser();
    void derivesABudgetFromTheModel();
    void appliesToATranscript();
};

void TestTrimPolicy::keepsEverythingWithoutLimits()
{
    const TrimPolicy policy;
    QVERIFY(!policy.hasLimits());
    QCOMPARE(policy.apply(conversation(50)).size(), 100);
}

void TestTrimPolicy::dropsTheOldestTurnsFirst()
{
    // Recent turns are what the model needs; the oldest are what it can spare.
    TrimPolicy policy;
    policy.setMaxMessages(3);

    QCOMPARE(contentsOf(policy.apply(conversation(3))),
             QStringList({QStringLiteral("a2"), QStringLiteral("q3"), QStringLiteral("a3")}));
}

void TestTrimPolicy::alwaysKeepsTheSystemPrompt()
{
    // Dropping it changes how the model behaves, which is not what trimming is
    // supposed to do -- and it counts against the budget, because the model
    // still sees it.
    TrimPolicy policy;
    policy.setMaxMessages(2);

    QList<Message> messages {Message::system(QStringLiteral("be terse"))};
    messages += conversation(3);

    QCOMPARE(contentsOf(policy.apply(messages)),
             QStringList({QStringLiteral("be terse"), QStringLiteral("a3")}));
}

void TestTrimPolicy::neverLetsAToolResultLead()
{
    // A tool result whose request was dropped answers nothing, and some
    // providers reject the conversation for it.
    QList<Message> messages {
            Message::user(QStringLiteral("q1")),
            Message(Role::Assistant, QStringLiteral("calling")),
            Message::toolResult(QStringLiteral("call_1"), QStringLiteral("result")),
            Message(Role::Assistant, QStringLiteral("a1")),
    };

    TrimPolicy policy;
    policy.setMaxMessages(2);

    // Two messages would start at the tool result, so it goes too.
    QCOMPARE(contentsOf(policy.apply(messages)), QStringList({QStringLiteral("a1")}));
}

void TestTrimPolicy::keepsTheNewestTurnEvenWhenItDoesNotFit()
{
    // Being told the message is too long is more useful than silently sending
    // an empty conversation.
    TrimPolicy policy;
    policy.setMaxTokens(1);

    const QList<Message> messages {Message::user(QString(400, QLatin1Char('x')))};
    QCOMPARE(policy.apply(messages).size(), 1);
}

void TestTrimPolicy::countsAgainstATokenBudget()
{
    // The default counter estimates four characters to the token, so each of
    // these is 5 tokens of content plus the per-message framing.
    TrimPolicy policy;
    policy.setMaxTokens(40);

    QList<Message> messages;
    for (int i = 1; i <= 10; ++i)
        messages.append(Message::user(QStringLiteral("%1").arg(i).repeated(20)));

    const QList<Message> kept = policy.apply(messages);
    QVERIFY(kept.size() < messages.size());
    QVERIFY(policy.tokenCounter().count(kept) <= 40);
    // What survived is the tail, not an arbitrary subset.
    QCOMPARE(kept.last().content(), messages.last().content());
}

void TestTrimPolicy::reservesRoomForTheReply()
{
    // The window has to hold the answer too, so a budget that ignores it is
    // not a budget.
    QList<Message> messages;
    for (int i = 1; i <= 10; ++i)
        messages.append(Message::user(QString(40, QLatin1Char('x'))));

    TrimPolicy generous;
    generous.setMaxTokens(200);

    TrimPolicy reserved;
    reserved.setMaxTokens(200);
    reserved.setReservedForReply(150);

    QVERIFY(reserved.apply(messages).size() < generous.apply(messages).size());
}

void TestTrimPolicy::handsTheDroppedMessagesToASummariser()
{
    TrimPolicy policy;
    policy.setMaxMessages(2);

    QList<Message> seen;
    policy.setSummariser([&seen](const QList<Message> &dropped) {
        seen = dropped;
        return Message::system(QStringLiteral("earlier: %1 messages").arg(dropped.size()));
    });

    const QList<Message> kept = policy.apply(conversation(3));

    QCOMPARE(seen.size(), 4);
    QCOMPARE(seen.first().content(), QStringLiteral("q1"));
    QCOMPARE(contentsOf(kept), QStringList({QStringLiteral("earlier: 4 messages"),
                                            QStringLiteral("q3"), QStringLiteral("a3")}));

    // A summariser that returns nothing drops them silently, which is the
    // behaviour without one.
    policy.setSummariser([](const QList<Message> &) { return Message(); });
    QCOMPARE(contentsOf(policy.apply(conversation(3))),
             QStringList({QStringLiteral("q3"), QStringLiteral("a3")}));
}

void TestTrimPolicy::derivesABudgetFromTheModel()
{
    // The catalog already knows the window and the output limit, so nobody has
    // to write the arithmetic out again.
    const TrimPolicy policy = TrimPolicy::forModel(QStringLiteral("gpt-4o-mini"));

    QCOMPARE(policy.maxTokens(), 128000);
    QCOMPARE(policy.reservedForReply(), 16384);
    QCOMPARE(policy.tokenCounter().encoding(), QStringLiteral("o200k_base"));
    QVERIFY(policy.hasLimits());
}

void TestTrimPolicy::appliesToATranscript()
{
    Transcript transcript;
    transcript.setSystemPrompt(QStringLiteral("be terse"));
    for (int i = 1; i <= 5; ++i)
        transcript.addUserMessage(QStringLiteral("q%1").arg(i));

    TrimPolicy policy;
    policy.setMaxMessages(3);
    transcript.setTrimPolicy(policy);

    // The tree keeps everything; only the context handed to the model is cut.
    QCOMPARE(transcript.count(), 5);
    QCOMPARE(contentsOf(transcript.messages()),
             QStringList({QStringLiteral("be terse"), QStringLiteral("q4"), QStringLiteral("q5")}));
    QCOMPARE(transcript.buildRequest(QStringLiteral("gpt-4o-mini")).messages().size(), 3);
}

QTEST_MAIN(TestTrimPolicy)
#include "tst_trimpolicy.moc"
