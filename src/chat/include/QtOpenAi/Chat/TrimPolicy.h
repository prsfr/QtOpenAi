// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Chat/GlobalChat.h>
#include <QtOpenAi/Core/Message.h>
#include <QtOpenAi/Core/TokenCounter.h>

#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>

#include <functional>

namespace QtOpenAi {
namespace Chat {

class TrimPolicyData;

// How a transcript is cut down to what the model will accept.
//
// A conversation grows without limit; a context window does not. Something has
// to go, and which something is a decision only the application can make, so
// this is a value handed to a Transcript rather than behaviour built into it.
//
//     TrimPolicy policy = TrimPolicy::forModel("gpt-4o-mini");
//     policy.setReservedForReply(2000);
//     transcript.setTrimPolicy(policy);
//
// Two limits, either or both: a message count and a token budget. Trimming
// drops from the oldest end, which is the one convention every client shares --
// recent turns are what the model needs, and the system prompt is what the
// application needs.
//
// Three invariants hold whatever the limits are:
//
//  * **The system prompt survives.** It is the instruction the whole
//    conversation runs under; dropping it changes the model's behaviour rather
//    than merely shortening its memory.
//  * **A tool result never leads.** Dropping an assistant turn that requested
//    tools would otherwise leave its results answering nothing, which some
//    providers reject outright.
//  * **The newest turn survives.** A single message longer than the budget is
//    kept anyway: sending it and being told it is too long beats silently
//    sending nothing.
//
// When even that is not enough, `setSummariser()` replaces what was dropped
// with one message of the application's making, which is where a "…and earlier
// you said" summary or a call back to the model belongs.
class QTOPENAI_CHAT_EXPORT TrimPolicy
{
public:
    // Replaces the dropped messages with a single message. Returning a message
    // with no content drops them silently, which is the default behaviour.
    using Summariser = std::function<Core::Message(const QList<Core::Message> &dropped)>;

    TrimPolicy(); // no limits: keep everything
    TrimPolicy(const TrimPolicy &other);
    TrimPolicy(TrimPolicy &&other) noexcept;
    TrimPolicy &operator=(const TrimPolicy &other);
    TrimPolicy &operator=(TrimPolicy &&other) noexcept;
    ~TrimPolicy();

    void swap(TrimPolicy &other) noexcept { d.swap(other.d); }

    // A budget derived from what the model actually accepts: its context window
    // less the room its longest reply would need, counted with its own
    // tokenizer.
    static TrimPolicy forModel(const QString &model);

    // Zero means no limit, for both.
    int maxMessages() const;
    void setMaxMessages(int count);

    int maxTokens() const;
    void setMaxTokens(int tokens);

    // Held back from maxTokens for the model's answer, since the window has to
    // fit both.
    int reservedForReply() const;
    void setReservedForReply(int tokens);

    Core::TokenCounter tokenCounter() const;
    void setTokenCounter(const Core::TokenCounter &counter);

    Summariser summariser() const;
    void setSummariser(Summariser summariser);

    bool hasLimits() const;

    // The messages that fit, oldest dropped first.
    QList<Core::Message> apply(const QList<Core::Message> &messages) const;

private:
    QSharedDataPointer<TrimPolicyData> d;
};

} // namespace Chat
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Chat::TrimPolicy)
