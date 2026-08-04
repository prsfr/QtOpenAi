// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/Guardrail.h"

#include "QtOpenAi/Client/Client.h"

#include <QtCore/QPointer>

namespace QtOpenAi {
namespace Client {

namespace {

// Worst first, so verdict.category() names the reason a caller would give a
// user rather than whichever category happened to sort first alphabetically.
QStringList byScoreDescending(QStringList categories, const QMap<QString, double> &scores)
{
    std::sort(categories.begin(), categories.end(), [&scores](const QString &a, const QString &b) {
        return scores.value(a) > scores.value(b);
    });
    return categories;
}

// The newest user message: the rest of a conversation was screened when it was
// new, and re-screening it every turn would bill for the same text repeatedly
// and grow more expensive the longer the conversation ran.
QString newestUserText(const QList<Core::Message> &messages)
{
    for (auto it = messages.crbegin(); it != messages.crend(); ++it) {
        if (it->role() == Core::Role::User)
            return it->content();
    }
    return {};
}

} // namespace

class GuardrailReplyPrivate
{
public:
    GuardrailVerdict verdict;
    ClientError error;
    bool finished = false;
};

GuardrailReply::GuardrailReply(QObject *parent)
    : QObject(parent)
    , d_ptr(new GuardrailReplyPrivate)
{ }

GuardrailReply::~GuardrailReply() = default;

bool GuardrailReply::isFinished() const
{
    Q_D(const GuardrailReply);
    return d->finished;
}

GuardrailVerdict GuardrailReply::verdict() const
{
    Q_D(const GuardrailReply);
    return d->verdict;
}

ClientError GuardrailReply::error() const
{
    Q_D(const GuardrailReply);
    return d->error;
}

class GuardedChatReplyPrivate
{
public:
    Core::ChatCompletionResponse response;
    ClientError error;
    bool finished = false;
    bool blocked = false;
};

GuardedChatReply::GuardedChatReply(QObject *parent)
    : QObject(parent)
    , d_ptr(new GuardedChatReplyPrivate)
{ }

GuardedChatReply::~GuardedChatReply() = default;

bool GuardedChatReply::isFinished() const
{
    Q_D(const GuardedChatReply);
    return d->finished;
}

bool GuardedChatReply::isBlocked() const
{
    Q_D(const GuardedChatReply);
    return d->blocked;
}

Core::ChatCompletionResponse GuardedChatReply::response() const
{
    Q_D(const GuardedChatReply);
    return d->response;
}

ClientError GuardedChatReply::error() const
{
    Q_D(const GuardedChatReply);
    return d->error;
}

class GuardrailPrivate
{
public:
    QPointer<Client> client;
    QMap<QString, GuardrailAction> actions;
    GuardrailAction defaultAction = GuardrailAction::Block;
    double threshold = 1.0;
    QString model;
    bool screenInput = true;
    bool screenOutput = true;
};

Guardrail::Guardrail(Client *client, QObject *parent)
    : QObject(parent)
    , d_ptr(new GuardrailPrivate)
{
    Q_D(Guardrail);
    d->client = client;
}

Guardrail::~Guardrail() = default;

Client *Guardrail::client() const
{
    Q_D(const Guardrail);
    return d->client;
}

void Guardrail::setAction(const QString &category, GuardrailAction action)
{
    Q_D(Guardrail);
    d->actions.insert(category, action);
}

GuardrailAction Guardrail::action(const QString &category) const
{
    Q_D(const Guardrail);
    return d->actions.value(category, d->defaultAction);
}

QMap<QString, GuardrailAction> Guardrail::actions() const
{
    Q_D(const Guardrail);
    return d->actions;
}

void Guardrail::clearActions()
{
    Q_D(Guardrail);
    d->actions.clear();
}

void Guardrail::setDefaultAction(GuardrailAction action)
{
    Q_D(Guardrail);
    d->defaultAction = action;
}

GuardrailAction Guardrail::defaultAction() const
{
    Q_D(const Guardrail);
    return d->defaultAction;
}

void Guardrail::setThreshold(double threshold)
{
    Q_D(Guardrail);
    d->threshold = qBound(0.0, threshold, 1.0);
}

double Guardrail::threshold() const
{
    Q_D(const Guardrail);
    return d->threshold;
}

QString Guardrail::model() const
{
    Q_D(const Guardrail);
    return d->model;
}

void Guardrail::setModel(const QString &model)
{
    Q_D(Guardrail);
    d->model = model;
}

void Guardrail::setScreenInput(bool enabled)
{
    Q_D(Guardrail);
    d->screenInput = enabled;
}

bool Guardrail::screensInput() const
{
    Q_D(const Guardrail);
    return d->screenInput;
}

void Guardrail::setScreenOutput(bool enabled)
{
    Q_D(Guardrail);
    d->screenOutput = enabled;
}

bool Guardrail::screensOutput() const
{
    Q_D(const Guardrail);
    return d->screenOutput;
}

GuardrailVerdict Guardrail::judge(const Core::ModerationResult &result) const
{
    Q_D(const Guardrail);

    GuardrailVerdict verdict;
    verdict.result = result;
    verdict.scores = result.categoryScores();

    const QMap<QString, bool> categories = result.categories();
    QStringList matched;
    for (auto it = categories.constBegin(); it != categories.constEnd(); ++it) {
        // Either the provider flagged it, or it scored past a threshold the
        // caller set because they want to be stricter than the provider is.
        const bool present
                = it.value()
                  || (d->threshold < 1.0 && verdict.scores.value(it.key()) >= d->threshold);
        if (!present)
            continue;

        const GuardrailAction action = this->action(it.key());
        if (action == GuardrailAction::Allow)
            continue; // present, but this policy does not care
        matched.append(it.key());
        // Block wins over Warn: the strictest matched category decides, because
        // letting a warned category downgrade a blocked one would make the
        // outcome depend on map ordering.
        if (action == GuardrailAction::Block)
            verdict.action = GuardrailAction::Block;
        else if (verdict.action == GuardrailAction::Allow)
            verdict.action = GuardrailAction::Warn;
    }

    verdict.categories = byScoreDescending(std::move(matched), verdict.scores);
    return verdict;
}

GuardrailReply *Guardrail::screen(const QString &text)
{
    Q_D(Guardrail);

    auto *reply = new GuardrailReply(this);

    if (!d->client) {
        // Report it rather than never finishing: a screening call that silently
        // never answers would hang whatever is waiting on the verdict.
        reply->d_func()->error = ClientError(ClientError::Kind::InvalidRequest,
                                             QStringLiteral("no client to screen with"));
        QMetaObject::invokeMethod(
                reply,
                [reply]() {
                    reply->d_func()->finished = true;
                    Q_EMIT reply->failed(reply->error());
                    Q_EMIT reply->done();
                },
                Qt::QueuedConnection);
        return reply;
    }

    Core::ModerationRequest request(text);
    if (!d->model.isEmpty())
        request.setModel(d->model);

    ModerationReply *moderation = d->client->createModeration(request);
    connect(moderation, &ModerationReply::finished, reply,
            [this, reply](const Core::ModerationResponse &response) {
                const GuardrailVerdict verdict = judge(response.firstResult());
                reply->d_func()->verdict = verdict;
                reply->d_func()->finished = true;
                if (verdict.isFlagged())
                    Q_EMIT flagged(verdict);
                Q_EMIT reply->finished(verdict);
                Q_EMIT reply->done();
            });
    connect(moderation, &ModerationReply::failed, reply, [reply](const ClientError &error) {
        reply->d_func()->error = error;
        reply->d_func()->finished = true;
        Q_EMIT reply->failed(error);
        Q_EMIT reply->done();
    });
    return reply;
}

GuardrailReply *Guardrail::screen(const QList<Core::Message> &messages)
{
    return screen(newestUserText(messages));
}

GuardedChatReply *Guardrail::createChatCompletion(const Core::ChatCompletionRequest &request)
{
    Q_D(Guardrail);

    auto *guarded = new GuardedChatReply(this);

    // The three steps, each written once and chained by the one before it.
    // Spelling them out as nested lambdas would put the failure handling three
    // levels deep for no gain.
    const auto settle = [guarded]() {
        guarded->d_func()->finished = true;
        Q_EMIT guarded->done();
    };
    const auto fail = [guarded, settle](const ClientError &error) {
        guarded->d_func()->error = error;
        Q_EMIT guarded->failed(error);
        settle();
    };
    const auto block = [guarded, settle](GuardedChatReply::Position position,
                                         const GuardrailVerdict &verdict) {
        guarded->d_func()->blocked = true;
        Q_EMIT guarded->blocked(position, verdict);
        settle();
    };

    // Step 3: screen the answer, then hand it over.
    const auto screenOutput = [this, d, guarded, settle, fail,
                               block](const Core::ChatCompletionResponse &response) {
        const auto deliver = [guarded, settle, response]() {
            guarded->d_func()->response = response;
            Q_EMIT guarded->finished(response);
            settle();
        };

        const QString answer = response.choices().value(0).message().content();
        if (!d->screenOutput || answer.isEmpty()) {
            deliver();
            return;
        }

        GuardrailReply *check = screen(answer);
        connect(check, &GuardrailReply::finished, guarded,
                [guarded, deliver, block](const GuardrailVerdict &verdict) {
                    if (verdict.isBlocked()) {
                        block(GuardedChatReply::Position::Output, verdict);
                        return;
                    }
                    // Warn goes out before the answer, so a caller can annotate
                    // what it is about to show rather than correct it after.
                    if (verdict.isFlagged())
                        Q_EMIT guarded->flagged(GuardedChatReply::Position::Output, verdict);
                    deliver();
                });
        connect(check, &GuardrailReply::failed, guarded, fail);
    };

    // Step 2: the request itself.
    const auto send = [d, guarded, fail, screenOutput, request]() {
        if (!d->client) {
            fail(ClientError(ClientError::Kind::InvalidRequest,
                             QStringLiteral("no client to send with")));
            return;
        }
        ChatCompletionReply *reply = d->client->createChatCompletion(request);
        connect(reply, &ChatCompletionReply::finished, guarded, screenOutput);
        connect(reply, &ChatCompletionReply::failed, guarded, fail);
    };

    // Step 1: screen what the user asked, before spending a request relaying
    // something this application would refuse to show.
    const QString prompt = newestUserText(request.messages());
    if (!d->screenInput || prompt.isEmpty()) {
        QMetaObject::invokeMethod(guarded, send, Qt::QueuedConnection);
        return guarded;
    }

    GuardrailReply *check = screen(prompt);
    connect(check, &GuardrailReply::finished, guarded,
            [guarded, send, block](const GuardrailVerdict &verdict) {
                if (verdict.isBlocked()) {
                    block(GuardedChatReply::Position::Input, verdict);
                    return;
                }
                if (verdict.isFlagged())
                    Q_EMIT guarded->flagged(GuardedChatReply::Position::Input, verdict);
                send();
            });
    connect(check, &GuardrailReply::failed, guarded, fail);

    return guarded;
}

} // namespace Client
} // namespace QtOpenAi
