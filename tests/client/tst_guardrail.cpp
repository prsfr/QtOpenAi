// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/Guardrail.h>

#include <QtTest/QtTest>

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

namespace {

// A moderation answer with one category set as asked. Written as a builder
// rather than as a pile of literals so each test states only what it is about.
QByteArray moderation(bool flagged, const QString &category = QStringLiteral("violence"),
                      double score = 0.9)
{
    return QStringLiteral(R"({"id":"modr-1","model":"omni-moderation-latest","results":[
        {"flagged":%1,"categories":{"%2":%1,"hate":false},
         "category_scores":{"%2":%3,"hate":0.01}}]})")
            .arg(flagged ? QStringLiteral("true") : QStringLiteral("false"), category)
            .arg(score)
            .toUtf8();
}

// Two categories at once, for the case where they disagree.
QByteArray moderationBoth()
{
    return R"({"id":"modr-1","model":"omni-moderation-latest","results":[
        {"flagged":true,"categories":{"violence":true,"hate":true},
         "category_scores":{"violence":0.6,"hate":0.95}}]})";
}

const char kCompletion[] = R"({"id":"c","object":"chat.completion","created":1,
    "model":"m","choices":[{"index":0,"finish_reason":"stop",
    "message":{"role":"assistant","content":"the answer"}}]})";

ChatCompletionRequest ask(const QString &prompt = QStringLiteral("a question"))
{
    return ChatCompletionRequest(QStringLiteral("m"), {Message::user(prompt)});
}

// Wait for a guarded exchange to settle and report whether it did.
bool settled(GuardedChatReply *reply, int timeoutMs = 5000)
{
    if (!reply)
        return false;
    QSignalSpy done(reply, &GuardedChatReply::done);
    return reply->isFinished() || done.wait(timeoutMs);
}

} // namespace

// Coverage for the moderation guardrail (#50).
class TestGuardrail : public QObject
{
    Q_OBJECT
private slots:
    void unflaggedContentPassesThrough();
    void aFlaggedInputIsBlockedBeforeTheRequest();
    void aFlaggedOutputIsBlockedAfterIt();
    void warnLetsItThroughAndSaysSo();
    void aCategoryThePolicyAllowsIsNotAMatch();
    void theStrictestMatchedCategoryDecides();
    void theThresholdCanBeStricterThanTheProvider();
    void screeningCanBeTurnedOff();
    void screenReportsAVerdictOnItsOwn();
    void aFailedScreeningFailsRatherThanPasses();
    void judgeIsThePolicyWithoutARequest();
};

void TestGuardrail::unflaggedContentPassesThrough()
{
    // Input screening, the request, output screening: three round trips, which
    // is what a screened exchange costs and worth being explicit about.
    StubServer server(
            QList<StubServer::Response> {{moderation(false)}, {kCompletion}, {moderation(false)}});
    Client client;
    client.setBaseUrl(server.baseUrl());

    Guardrail guardrail(&client);
    QSignalSpy flagged(&guardrail, &Guardrail::flagged);

    auto *reply = guardrail.createChatCompletion(ask());
    QSignalSpy finished(reply, &GuardedChatReply::finished);
    QVERIFY(settled(reply));

    QVERIFY(!reply->isBlocked());
    QCOMPARE(finished.count(), 1);
    QCOMPARE(reply->response().choices().at(0).message().content(), QStringLiteral("the answer"));
    QCOMPARE(server.requestCount(), 3);
    QCOMPARE(flagged.count(), 0);
}

void TestGuardrail::aFlaggedInputIsBlockedBeforeTheRequest()
{
    // The point of screening the input: not spending a request relaying
    // something this application would refuse to show.
    StubServer server(QList<StubServer::Response> {{moderation(true)}, {kCompletion}});
    Client client;
    client.setBaseUrl(server.baseUrl());

    Guardrail guardrail(&client);
    auto *reply = guardrail.createChatCompletion(ask());
    QSignalSpy blocked(reply, &GuardedChatReply::blocked);
    QSignalSpy finished(reply, &GuardedChatReply::finished);
    QVERIFY(settled(reply));

    QVERIFY(reply->isBlocked());
    QCOMPARE(finished.count(), 0);
    QCOMPARE(blocked.count(), 1);
    QCOMPARE(blocked.first().at(0).value<GuardedChatReply::Position>(),
             GuardedChatReply::Position::Input);

    const auto verdict = blocked.first().at(1).value<GuardrailVerdict>();
    QCOMPARE(verdict.action, GuardrailAction::Block);
    QCOMPARE(verdict.category(), QStringLiteral("violence"));
    QCOMPARE(verdict.score(), 0.9);

    // One request, not two: the chat completion was never sent.
    QCOMPARE(server.requestCount(), 1);
}

void TestGuardrail::aFlaggedOutputIsBlockedAfterIt()
{
    // A model can produce what its prompt did not ask for, so a clean input is
    // not a reason to skip the answer.
    StubServer server(
            QList<StubServer::Response> {{moderation(false)}, {kCompletion}, {moderation(true)}});
    Client client;
    client.setBaseUrl(server.baseUrl());

    Guardrail guardrail(&client);
    auto *reply = guardrail.createChatCompletion(ask());
    QSignalSpy blocked(reply, &GuardedChatReply::blocked);
    QSignalSpy finished(reply, &GuardedChatReply::finished);
    QVERIFY(settled(reply));

    QVERIFY(reply->isBlocked());
    QCOMPARE(finished.count(), 0);
    QCOMPARE(blocked.count(), 1);
    QCOMPARE(blocked.first().at(0).value<GuardedChatReply::Position>(),
             GuardedChatReply::Position::Output);
    // Blocked means not handed over, even though it did arrive.
    QVERIFY(reply->response().choices().isEmpty());
    QCOMPARE(server.requestCount(), 3);
}

void TestGuardrail::warnLetsItThroughAndSaysSo()
{
    StubServer server(
            QList<StubServer::Response> {{moderation(true)}, {kCompletion}, {moderation(false)}});
    Client client;
    client.setBaseUrl(server.baseUrl());

    Guardrail guardrail(&client);
    guardrail.setAction(QStringLiteral("violence"), GuardrailAction::Warn);

    auto *reply = guardrail.createChatCompletion(ask());
    QSignalSpy flagged(reply, &GuardedChatReply::flagged);
    QSignalSpy finished(reply, &GuardedChatReply::finished);
    QVERIFY(settled(reply));

    // Through, and annotated: an application that wants to show a notice needs
    // both, and needs the notice before the content it is about.
    QVERIFY(!reply->isBlocked());
    QCOMPARE(finished.count(), 1);
    QCOMPARE(flagged.count(), 1);
    QCOMPARE(flagged.first().at(0).value<GuardedChatReply::Position>(),
             GuardedChatReply::Position::Input);
    QCOMPARE(flagged.first().at(1).value<GuardrailVerdict>().action, GuardrailAction::Warn);
    QCOMPARE(server.requestCount(), 3);
}

void TestGuardrail::aCategoryThePolicyAllowsIsNotAMatch()
{
    // The categories are not comparable: an app for adults may reasonably allow
    // what a children's app must block, and neither is a "level" of the other.
    StubServer server(
            QList<StubServer::Response> {{moderation(true)}, {kCompletion}, {moderation(true)}});
    Client client;
    client.setBaseUrl(server.baseUrl());

    Guardrail guardrail(&client);
    guardrail.setAction(QStringLiteral("violence"), GuardrailAction::Allow);
    QCOMPARE(guardrail.action(QStringLiteral("violence")), GuardrailAction::Allow);
    // Anything unnamed still gets the default, which is Block: a category
    // nobody thought about is more likely to matter than not.
    QCOMPARE(guardrail.defaultAction(), GuardrailAction::Block);
    QCOMPARE(guardrail.action(QStringLiteral("never-heard-of-it")), GuardrailAction::Block);

    auto *reply = guardrail.createChatCompletion(ask());
    QSignalSpy finished(reply, &GuardedChatReply::finished);
    QVERIFY(settled(reply));
    QVERIFY(!reply->isBlocked());
    QCOMPARE(finished.count(), 1);

    guardrail.clearActions();
    QVERIFY(guardrail.actions().isEmpty());
    QCOMPARE(guardrail.action(QStringLiteral("violence")), GuardrailAction::Block);
}

void TestGuardrail::theStrictestMatchedCategoryDecides()
{
    // Both categories match; one warns, one blocks. Letting the warn downgrade
    // the block would make the outcome depend on map ordering.
    StubServer server(QList<StubServer::Response> {{moderationBoth()}, {kCompletion}});
    Client client;
    client.setBaseUrl(server.baseUrl());

    Guardrail guardrail(&client);
    guardrail.setAction(QStringLiteral("violence"), GuardrailAction::Warn);
    guardrail.setAction(QStringLiteral("hate"), GuardrailAction::Block);

    auto *reply = guardrail.createChatCompletion(ask());
    QSignalSpy blocked(reply, &GuardedChatReply::blocked);
    QVERIFY(settled(reply));

    QVERIFY(reply->isBlocked());
    const auto verdict = blocked.first().at(1).value<GuardrailVerdict>();
    QCOMPARE(verdict.action, GuardrailAction::Block);
    // Worst first, so the reason a user is given is the strongest one rather
    // than whichever sorted first alphabetically.
    QCOMPARE(verdict.categories, QStringList({QStringLiteral("hate"), QStringLiteral("violence")}));
    QCOMPARE(verdict.category(), QStringLiteral("hate"));
}

void TestGuardrail::theThresholdCanBeStricterThanTheProvider()
{
    // The provider says it is fine; this policy says 0.8 is not.
    StubServer server(QList<StubServer::Response> {
            {moderation(false, QStringLiteral("violence"), 0.8)}, {kCompletion}});
    Client client;
    client.setBaseUrl(server.baseUrl());

    Guardrail guardrail(&client);
    QCOMPARE(guardrail.threshold(), 1.0); // trust the provider's flag and nothing else
    guardrail.setThreshold(0.5);

    auto *reply = guardrail.createChatCompletion(ask());
    QVERIFY(settled(reply));
    QVERIFY(reply->isBlocked());
    QCOMPARE(server.requestCount(), 1);

    // Out of range is clamped rather than rejected: there is no sensible
    // behaviour for a threshold of 5, and refusing to set it would leave the
    // guardrail in whatever state the caller thought they had changed.
    guardrail.setThreshold(5.0);
    QCOMPARE(guardrail.threshold(), 1.0);
}

void TestGuardrail::screeningCanBeTurnedOff()
{
    StubServer server(QList<StubServer::Response> {{kCompletion}});
    Client client;
    client.setBaseUrl(server.baseUrl());

    Guardrail guardrail(&client);
    QVERIFY(guardrail.screensInput());
    QVERIFY(guardrail.screensOutput());
    guardrail.setScreenInput(false);
    guardrail.setScreenOutput(false);

    auto *reply = guardrail.createChatCompletion(ask());
    QSignalSpy finished(reply, &GuardedChatReply::finished);
    QVERIFY(settled(reply));

    QCOMPARE(finished.count(), 1);
    // One request: with both checks off this is an ordinary chat completion.
    QCOMPARE(server.requestCount(), 1);
}

void TestGuardrail::screenReportsAVerdictOnItsOwn()
{
    // Screening text without sending anything -- what an application does to a
    // draft before the user has committed to it.
    StubServer server(QList<StubServer::Response> {{moderation(true)}, {moderation(false)}});
    Client client;
    client.setBaseUrl(server.baseUrl());

    Guardrail guardrail(&client);
    QSignalSpy flagged(&guardrail, &Guardrail::flagged);

    auto *first = guardrail.screen(QStringLiteral("something"));
    QSignalSpy firstDone(first, &GuardrailReply::done);
    QVERIFY(firstDone.wait(5000));
    QVERIFY(first->isFinished());
    QVERIFY(first->verdict().isBlocked());
    QCOMPARE(first->verdict().scores.value(QStringLiteral("violence")), 0.9);
    // Every non-Allow verdict, wherever it came from, on one connection.
    QCOMPARE(flagged.count(), 1);

    // The overload that takes messages screens the newest user turn: the rest
    // was screened when it was new, and re-screening it every turn would bill
    // for the same text again and again.
    auto *second = guardrail.screen(QList<Message> {Message::user(QStringLiteral("old")),
                                                    Message::assistant(QStringLiteral("reply")),
                                                    Message::user(QStringLiteral("new"))});
    QSignalSpy secondDone(second, &GuardrailReply::done);
    QVERIFY(secondDone.wait(5000));
    QVERIFY(!second->verdict().isFlagged());
    QVERIFY(server.requestBodies().at(1).contains("new"));
    QVERIFY(!server.requestBodies().at(1).contains("old"));
}

void TestGuardrail::aFailedScreeningFailsRatherThanPasses()
{
    // A guardrail that treats "I could not check" as "it is fine" is not a
    // guardrail. The exchange fails; it does not quietly go through.
    StubServer server(500, R"({"error":{"message":"moderation is down"}})");
    Client client;
    client.setBaseUrl(server.baseUrl());
    client.setRetryPolicy(RetryPolicy::none());

    Guardrail guardrail(&client);
    auto *reply = guardrail.createChatCompletion(ask());
    QSignalSpy failed(reply, &GuardedChatReply::failed);
    QSignalSpy finished(reply, &GuardedChatReply::finished);
    QVERIFY(settled(reply));

    QCOMPARE(failed.count(), 1);
    QCOMPARE(finished.count(), 0);
    QVERIFY(!reply->isBlocked());
    QCOMPARE(reply->error().message(), QStringLiteral("moderation is down"));

    // A guardrail with no client says so rather than never answering.
    Guardrail orphan(nullptr);
    auto *orphaned = orphan.screen(QStringLiteral("text"));
    QSignalSpy orphanDone(orphaned, &GuardrailReply::done);
    QVERIFY(orphanDone.wait(2000));
    QCOMPARE(orphaned->error().kind(), ClientError::Kind::InvalidRequest);
}

void TestGuardrail::judgeIsThePolicyWithoutARequest()
{
    // One decision procedure, reachable on its own: a moderation answer got
    // some other way has to reach the same verdict, or the policy would mean
    // two different things depending on how it was asked.
    Guardrail guardrail(nullptr);
    guardrail.setAction(QStringLiteral("hate"), GuardrailAction::Warn);

    ModerationResult result;
    result.setFlagged(true);
    result.setCategories({{QStringLiteral("hate"), true}, {QStringLiteral("violence"), false}});
    result.setCategoryScores({{QStringLiteral("hate"), 0.7}, {QStringLiteral("violence"), 0.2}});

    const GuardrailVerdict verdict = guardrail.judge(result);
    QCOMPARE(verdict.action, GuardrailAction::Warn);
    QCOMPARE(verdict.categories, QStringList({QStringLiteral("hate")}));
    QCOMPARE(verdict.score(), 0.7);
    // Every category the provider scored, not only the ones that matched, so a
    // caller can show a breakdown.
    QCOMPARE(verdict.scores.size(), 2);
    QCOMPARE(verdict.result, result);

    // Nothing flagged is Allow, whatever the default action is.
    ModerationResult clean;
    clean.setCategories({{QStringLiteral("hate"), false}});
    clean.setCategoryScores({{QStringLiteral("hate"), 0.01}});
    QCOMPARE(guardrail.judge(clean).action, GuardrailAction::Allow);
    QVERIFY(!guardrail.judge(clean).isFlagged());
}

QTEST_MAIN(TestGuardrail)
#include "tst_guardrail.moc"
