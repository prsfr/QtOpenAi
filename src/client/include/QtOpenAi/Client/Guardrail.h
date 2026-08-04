// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Core/ChatCompletionRequest.h>
#include <QtOpenAi/Core/ChatCompletionResponse.h>
#include <QtOpenAi/Core/ModerationResponse.h>

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace QtOpenAi {
namespace Client {

class Client;
class GuardrailPrivate;
class GuardrailReplyPrivate;
class GuardedChatReplyPrivate;

// What a policy decides about one piece of text.
enum class GuardrailAction {
    Allow, // nothing matched, or the policy does not care about what did
    Warn,  // matched, but the text still goes through -- annotate and continue
    Block  // matched; the text does not go through
};

// One screening decision, and everything the caller needs to explain it. A
// verdict that only said "blocked" would leave an application unable to tell a
// user *why*, which is the difference between a moderation feature and a wall.
struct QTOPENAI_CLIENT_EXPORT GuardrailVerdict
{
    GuardrailAction action = GuardrailAction::Allow;
    // The categories that matched the policy, worst first.
    QStringList categories;
    // Every category the provider scored, so a caller can show a breakdown
    // rather than only the categories that happened to cross the line.
    QMap<QString, double> scores;
    // The provider's answer, verbatim, for anyone who needs more than this.
    Core::ModerationResult result;

    bool isBlocked() const { return action == GuardrailAction::Block; }
    bool isFlagged() const { return action != GuardrailAction::Allow; }
    // The worst category that matched, or an empty string.
    QString category() const { return categories.value(0); }
    double score() const { return scores.value(category()); }
};

// The pending result of one screening call.
class QTOPENAI_CLIENT_EXPORT GuardrailReply : public QObject
{
    Q_OBJECT
public:
    ~GuardrailReply() override;

    bool isFinished() const;
    GuardrailVerdict verdict() const;
    ClientError error() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Client::GuardrailVerdict &verdict);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();

private:
    friend class Guardrail;
    explicit GuardrailReply(QObject *parent = nullptr);
    Q_DECLARE_PRIVATE(GuardrailReply)
    QScopedPointer<GuardrailReplyPrivate> d_ptr;
};

// A chat completion with the guardrail wrapped around it, both ways.
class QTOPENAI_CLIENT_EXPORT GuardedChatReply : public QObject
{
    Q_OBJECT
public:
    // Which side of the exchange a verdict is about.
    enum class Position {
        Input, // what the user asked
        Output // what the model answered
    };
    Q_ENUM(Position)

    ~GuardedChatReply() override;

    bool isFinished() const;
    bool isBlocked() const;
    Core::ChatCompletionResponse response() const;
    ClientError error() const;

Q_SIGNALS:
    // The model's answer, screened and allowed through.
    void finished(const QtOpenAi::Core::ChatCompletionResponse &response);
    // The policy said Block. No answer follows -- when the input is blocked the
    // request is never sent, and when the output is blocked it is not handed
    // over. `done()` still fires.
    void blocked(QtOpenAi::Client::GuardedChatReply::Position position,
                 const QtOpenAi::Client::GuardrailVerdict &verdict);
    // The policy said Warn: this went through, and here is what it matched.
    // Emitted before finished(), so a caller can annotate what it is about to
    // show rather than having to correct it afterwards.
    void flagged(QtOpenAi::Client::GuardedChatReply::Position position,
                 const QtOpenAi::Client::GuardrailVerdict &verdict);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();

private:
    friend class Guardrail;
    explicit GuardedChatReply(QObject *parent = nullptr);
    Q_DECLARE_PRIVATE(GuardedChatReply)
    QScopedPointer<GuardedChatReplyPrivate> d_ptr;
};

// Screens text against the Moderations API and applies a policy to the answer.
//
//     Guardrail guardrail(&client);
//     guardrail.setAction("self-harm", GuardrailAction::Block);
//
//     auto *reply = guardrail.createChatCompletion(request);
//     connect(reply, &GuardedChatReply::blocked, this, &Ui::showRefusal);
//     connect(reply, &GuardedChatReply::finished, this, &Ui::showAnswer);
//
// Both sides are worth screening and for different reasons: the input, so an
// application does not spend a request relaying something it would refuse to
// show; the output, because a model can produce what its prompt did not ask
// for. Either check can be turned off.
//
// **This is deliberately not an Interceptor.** The interceptor chain is
// synchronous -- a request either goes out now or is answered now -- and
// screening needs a round trip of its own. Wiring an awaited call into a chain
// that cannot wait would mean either blocking the event loop or letting the
// unscreened request go out first, and both defeat the purpose. So it composes
// calls instead, which is honest about what it costs: a screened exchange is
// two or three requests, not one.
//
// The policy is per category, because the categories are not comparable. An app
// for adults may reasonably allow what a children's app must block, and neither
// is a "level" of the other. Categories are provider-defined strings; a
// category the policy does not name gets defaultAction().
class QTOPENAI_CLIENT_EXPORT Guardrail : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool screenInput READ screensInput WRITE setScreenInput)
    Q_PROPERTY(bool screenOutput READ screensOutput WRITE setScreenOutput)
    Q_PROPERTY(double threshold READ threshold WRITE setThreshold)
public:
    explicit Guardrail(Client *client, QObject *parent = nullptr);
    ~Guardrail() override;

    Client *client() const;

    // What to do about a category the provider reports. Categories are the
    // provider's own strings ("hate", "violence", "self-harm", ...).
    void setAction(const QString &category, GuardrailAction action);
    GuardrailAction action(const QString &category) const;
    QMap<QString, GuardrailAction> actions() const;
    void clearActions();

    // Applied to any category the policy does not name. Defaults to Block:
    // a category nobody thought about is more likely to be one that matters
    // than one that does not, and an application that wants everything through
    // can say so in one line.
    void setDefaultAction(GuardrailAction action);
    GuardrailAction defaultAction() const;

    // A category counts as matched when the provider flags it, or when its
    // score reaches this. 1.0 (the default) means "trust the provider's own
    // flag and nothing else"; lowering it makes the policy stricter than the
    // provider is.
    void setThreshold(double threshold);
    double threshold() const;

    // The moderation model. Empty leaves it to the provider's default.
    QString model() const;
    void setModel(const QString &model);

    // Which sides to screen. Both on by default.
    void setScreenInput(bool enabled);
    bool screensInput() const;
    void setScreenOutput(bool enabled);
    bool screensOutput() const;

    // Screen one piece of text on its own.
    GuardrailReply *screen(const QString &text);
    // Screen the text of a conversation -- in practice the newest user message,
    // since the rest was screened when it was new.
    GuardrailReply *screen(const QList<Core::Message> &messages);

    // Apply the policy to `result` without making a request. The whole decision
    // procedure in one function, so screening a moderation answer obtained some
    // other way gives the same verdict as screening it through here.
    GuardrailVerdict judge(const Core::ModerationResult &result) const;

    // Screen, send, screen. The convenience the rest of this class exists for.
    GuardedChatReply *createChatCompletion(const Core::ChatCompletionRequest &request);

Q_SIGNALS:
    // Every non-Allow verdict this guardrail reaches, wherever it came from.
    // One connection is enough to log or count them all.
    void flagged(const QtOpenAi::Client::GuardrailVerdict &verdict);

private:
    Q_DECLARE_PRIVATE(Guardrail)
    QScopedPointer<GuardrailPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi

Q_DECLARE_METATYPE(QtOpenAi::Client::GuardrailVerdict)
