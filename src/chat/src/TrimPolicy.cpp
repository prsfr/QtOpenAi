// SPDX-License-Identifier: MIT
#include "QtOpenAi/Chat/TrimPolicy.h"

#include <QtOpenAi/Core/ModelCatalog.h>

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Chat {

namespace {

bool isSystem(const Core::Message &message)
{
    return message.role() == Core::Role::System || message.role() == Core::Role::Developer;
}

} // namespace

class TrimPolicyData : public QSharedData
{
public:
    int maxMessages = 0;
    int maxTokens = 0;
    int reservedForReply = 0;
    Core::TokenCounter counter;
    TrimPolicy::Summariser summariser;
};

TrimPolicy::TrimPolicy()
    : d(new TrimPolicyData)
{ }

TrimPolicy::TrimPolicy(const TrimPolicy &other) = default;
TrimPolicy::TrimPolicy(TrimPolicy &&other) noexcept = default;
TrimPolicy &TrimPolicy::operator=(const TrimPolicy &other) = default;
TrimPolicy &TrimPolicy::operator=(TrimPolicy &&other) noexcept = default;
TrimPolicy::~TrimPolicy() = default;

TrimPolicy TrimPolicy::forModel(const QString &model)
{
    const Core::ModelInfo info = Core::ModelCatalog::shared().model(model);

    TrimPolicy policy;
    policy.setMaxTokens(info.contextWindow());
    // The window has to hold the answer too, so the model's own output limit is
    // the natural amount to hold back.
    policy.setReservedForReply(info.maxOutputTokens());
    policy.setTokenCounter(Core::TokenCounter::forModel(model));
    return policy;
}

int TrimPolicy::maxMessages() const { return d->maxMessages; }
void TrimPolicy::setMaxMessages(int count) { d->maxMessages = count; }

int TrimPolicy::maxTokens() const { return d->maxTokens; }
void TrimPolicy::setMaxTokens(int tokens) { d->maxTokens = tokens; }

int TrimPolicy::reservedForReply() const { return d->reservedForReply; }
void TrimPolicy::setReservedForReply(int tokens) { d->reservedForReply = tokens; }

Core::TokenCounter TrimPolicy::tokenCounter() const { return d->counter; }
void TrimPolicy::setTokenCounter(const Core::TokenCounter &counter) { d->counter = counter; }

TrimPolicy::Summariser TrimPolicy::summariser() const { return d->summariser; }
void TrimPolicy::setSummariser(Summariser summariser) { d->summariser = std::move(summariser); }

bool TrimPolicy::hasLimits() const { return d->maxMessages > 0 || d->maxTokens > 0; }

QList<Core::Message> TrimPolicy::apply(const QList<Core::Message> &messages) const
{
    if (!hasLimits() || messages.isEmpty())
        return messages;

    // The system prompt is pinned, so it is set aside rather than trimmed --
    // and it still counts against the budget, because the model still sees it.
    QList<Core::Message> pinned;
    QList<Core::Message> turns;
    for (const Core::Message &message : messages) {
        if (isSystem(message) && turns.isEmpty())
            pinned.append(message);
        else
            turns.append(message);
    }

    const int tokenBudget = d->maxTokens > 0 ? d->maxTokens - d->reservedForReply : 0;

    // How many of the newest turns fit. Counting from the newest backwards
    // answers that directly, where counting forwards would only say when the
    // budget ran out.
    int keptFrom = turns.size();
    for (int i = turns.size() - 1; i >= 0; --i) {
        const int count = turns.size() - i;
        if (d->maxMessages > 0 && count + pinned.size() > d->maxMessages)
            break;
        if (tokenBudget > 0
            && d->counter.count(pinned + turns.mid(i)) > tokenBudget
            // The newest turn goes in whether it fits or not: being told it is
            // too long is more useful than sending nothing.
            && i != turns.size() - 1)
            break;
        keptFrom = i;
    }

    // A tool result answering a request that was dropped answers nothing, and
    // some providers reject the conversation outright for it.
    while (keptFrom < turns.size() && turns.at(keptFrom).role() == Core::Role::Tool)
        ++keptFrom;

    QList<Core::Message> result = pinned;
    if (keptFrom > 0 && d->summariser) {
        const Core::Message summary = d->summariser(turns.mid(0, keptFrom));
        if (summary.hasContent())
            result.append(summary);
    }
    result += turns.mid(keptFrom);
    return result;
}

} // namespace Chat
} // namespace QtOpenAi
