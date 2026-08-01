// SPDX-License-Identifier: MIT
#include "QtOpenAi/Chat/ConversationModel.h"

namespace QtOpenAi {
namespace Chat {

class ConversationModelPrivate
{
public:
    Transcript transcript;
};

ConversationModel::ConversationModel(QObject *parent)
    : QObject(parent)
    , d_ptr(new ConversationModelPrivate)
{ }

ConversationModel::~ConversationModel() = default;

Transcript ConversationModel::transcript() const
{
    Q_D(const ConversationModel);
    return d->transcript;
}

void ConversationModel::setTranscript(const Transcript &transcript)
{
    Q_D(ConversationModel);
    const QString oldPrompt = d->transcript.systemPrompt();
    const int oldCount = d->transcript.count();

    // A wholesale replacement keeps the trim policy, which belongs to this
    // model's configuration rather than to the history being loaded.
    const TrimPolicy policy = d->transcript.trimPolicy();
    d->transcript = transcript;
    d->transcript.setTrimPolicy(policy);

    Q_EMIT reset();
    if (d->transcript.systemPrompt() != oldPrompt)
        Q_EMIT systemPromptChanged(d->transcript.systemPrompt());
    if (d->transcript.count() != oldCount)
        Q_EMIT countChanged(d->transcript.count());
    Q_EMIT activeBranchChanged(d->transcript.activeLeaf());
}

QString ConversationModel::systemPrompt() const
{
    Q_D(const ConversationModel);
    return d->transcript.systemPrompt();
}

void ConversationModel::setSystemPrompt(const QString &prompt)
{
    Q_D(ConversationModel);
    if (d->transcript.systemPrompt() == prompt)
        return;
    d->transcript.setSystemPrompt(prompt);
    Q_EMIT systemPromptChanged(prompt);
}

TrimPolicy ConversationModel::trimPolicy() const
{
    Q_D(const ConversationModel);
    return d->transcript.trimPolicy();
}

void ConversationModel::setTrimPolicy(const TrimPolicy &policy)
{
    Q_D(ConversationModel);
    d->transcript.setTrimPolicy(policy);
}

int ConversationModel::count() const
{
    Q_D(const ConversationModel);
    return d->transcript.count();
}

Transcript::NodeId ConversationModel::activeLeaf() const
{
    Q_D(const ConversationModel);
    return d->transcript.activeLeaf();
}

Core::Message ConversationModel::message(Transcript::NodeId node) const
{
    Q_D(const ConversationModel);
    return d->transcript.message(node);
}

QList<Transcript::NodeId> ConversationModel::children(Transcript::NodeId node) const
{
    Q_D(const ConversationModel);
    return d->transcript.children(node);
}

QList<Transcript::NodeId> ConversationModel::siblings(Transcript::NodeId node) const
{
    Q_D(const ConversationModel);
    return d->transcript.siblings(node);
}

QList<Transcript::NodeId> ConversationModel::activePath() const
{
    Q_D(const ConversationModel);
    return d->transcript.activePath();
}

QList<Core::Message> ConversationModel::messages() const
{
    Q_D(const ConversationModel);
    return d->transcript.messages();
}

Core::ChatCompletionRequest ConversationModel::buildRequest(const QString &model) const
{
    Q_D(const ConversationModel);
    return d->transcript.buildRequest(model);
}

Transcript::NodeId ConversationModel::addUserMessage(const QString &text)
{
    return addMessage(Core::Message::user(text));
}

Transcript::NodeId ConversationModel::addMessage(const Core::Message &message)
{
    Q_D(ConversationModel);
    const Transcript::NodeId parent = d->transcript.activeLeaf();
    const Transcript::NodeId node = d->transcript.addMessage(message);

    Q_EMIT messageAdded(node, parent);
    Q_EMIT countChanged(d->transcript.count());
    Q_EMIT activeBranchChanged(node);
    return node;
}

Transcript::NodeId ConversationModel::fork(Transcript::NodeId node, const Core::Message &message)
{
    Q_D(ConversationModel);
    const Transcript::NodeId created = d->transcript.fork(node, message);
    if (created == Transcript::InvalidNode)
        return created;

    Q_EMIT messageAdded(created, d->transcript.parent(created));
    Q_EMIT countChanged(d->transcript.count());
    // The whole path below the fork point is different now, not just longer.
    Q_EMIT activeBranchChanged(created);
    return created;
}

bool ConversationModel::setActiveLeaf(Transcript::NodeId node)
{
    Q_D(ConversationModel);
    if (d->transcript.activeLeaf() == node)
        return true;
    if (!d->transcript.setActiveLeaf(node))
        return false;
    Q_EMIT activeBranchChanged(node);
    return true;
}

void ConversationModel::clear()
{
    Q_D(ConversationModel);
    if (d->transcript.isEmpty())
        return;
    d->transcript.clear();
    Q_EMIT reset();
    Q_EMIT countChanged(0);
    Q_EMIT activeBranchChanged(Transcript::InvalidNode);
}

} // namespace Chat
} // namespace QtOpenAi
