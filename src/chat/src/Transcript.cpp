// SPDX-License-Identifier: MIT
#include "QtOpenAi/Chat/Transcript.h"

#include <QtCore/QHash>
#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Chat {

namespace {

constexpr QLatin1String kNodes("nodes");
constexpr QLatin1String kId("id");
constexpr QLatin1String kParent("parent");
constexpr QLatin1String kMessage("message");
constexpr QLatin1String kActiveLeaf("active_leaf");
constexpr QLatin1String kSystemPrompt("system_prompt");

} // namespace

class TranscriptData : public QSharedData
{
public:
    struct Node
    {
        Transcript::NodeId parent = Transcript::InvalidNode;
        Core::Message message;
        QList<Transcript::NodeId> children;
    };

    QString systemPrompt;
    QHash<Transcript::NodeId, Node> nodes;
    QList<Transcript::NodeId> roots;
    Transcript::NodeId activeLeaf = Transcript::InvalidNode;
    Transcript::NodeId nextId = 1;
    TrimPolicy policy;

    Transcript::NodeId append(Transcript::NodeId parent, const Core::Message &message)
    {
        const Transcript::NodeId id = nextId++;
        Node node;
        node.parent = parent;
        node.message = message;
        nodes.insert(id, node);

        if (parent == Transcript::InvalidNode)
            roots.append(id);
        else
            nodes[parent].children.append(id);

        activeLeaf = id;
        return id;
    }
};

Transcript::Transcript()
    : d(new TranscriptData)
{ }

Transcript::Transcript(const Transcript &other) = default;
Transcript::Transcript(Transcript &&other) noexcept = default;
Transcript &Transcript::operator=(const Transcript &other) = default;
Transcript &Transcript::operator=(Transcript &&other) noexcept = default;
Transcript::~Transcript() = default;

QString Transcript::systemPrompt() const { return d->systemPrompt; }
void Transcript::setSystemPrompt(const QString &prompt) { d->systemPrompt = prompt; }

Transcript::NodeId Transcript::addMessage(const Core::Message &message)
{
    return d->append(d->activeLeaf, message);
}

Transcript::NodeId Transcript::addUserMessage(const QString &text)
{
    return addMessage(Core::Message::user(text));
}

QList<Transcript::NodeId> Transcript::addMessages(const QList<Core::Message> &messages)
{
    QList<NodeId> ids;
    ids.reserve(messages.size());
    for (const Core::Message &message : messages)
        ids.append(addMessage(message));
    return ids;
}

Transcript::NodeId Transcript::fork(NodeId node, const Core::Message &message)
{
    if (!d->nodes.contains(node))
        return InvalidNode;
    // A sibling, not a child: the new message takes the old one's place in the
    // conversation rather than following it.
    return d->append(d->nodes.value(node).parent, message);
}

Transcript::NodeId Transcript::activeLeaf() const { return d->activeLeaf; }

bool Transcript::setActiveLeaf(NodeId node)
{
    if (node != InvalidNode && !d->nodes.contains(node))
        return false;
    d->activeLeaf = node;
    return true;
}

Transcript::NodeId Transcript::parent(NodeId node) const { return d->nodes.value(node).parent; }

QList<Transcript::NodeId> Transcript::children(NodeId node) const
{
    return node == InvalidNode ? d->roots : d->nodes.value(node).children;
}

QList<Transcript::NodeId> Transcript::siblings(NodeId node) const
{
    if (!d->nodes.contains(node))
        return {};
    return children(d->nodes.value(node).parent);
}

QList<Transcript::NodeId> Transcript::roots() const { return d->roots; }

Core::Message Transcript::message(NodeId node) const { return d->nodes.value(node).message; }

bool Transcript::setMessage(NodeId node, const Core::Message &message)
{
    if (!d->nodes.contains(node))
        return false;
    d->nodes[node].message = message;
    return true;
}

bool Transcript::contains(NodeId node) const { return d->nodes.contains(node); }

int Transcript::count() const { return d->nodes.size(); }

bool Transcript::isEmpty() const { return d->nodes.isEmpty(); }

QList<Transcript::NodeId> Transcript::activePath() const
{
    QList<NodeId> path;
    for (NodeId node = d->activeLeaf; node != InvalidNode; node = d->nodes.value(node).parent)
        path.prepend(node);
    return path;
}

TrimPolicy Transcript::trimPolicy() const { return d->policy; }
void Transcript::setTrimPolicy(const TrimPolicy &policy) { d->policy = policy; }

QList<Core::Message> Transcript::messages() const
{
    QList<Core::Message> messages;
    if (!d->systemPrompt.isEmpty())
        messages.append(Core::Message::system(d->systemPrompt));
    for (const NodeId node : activePath())
        messages.append(d->nodes.value(node).message);
    return d->policy.apply(messages);
}

Core::ChatCompletionRequest Transcript::buildRequest(const QString &model) const
{
    return Core::ChatCompletionRequest(model, messages());
}

QJsonObject Transcript::toJson() const
{
    QJsonArray nodes;
    // Written parent-first, so fromJson() can rebuild the tree in one pass.
    for (NodeId id = 1; id < d->nextId; ++id) {
        auto it = d->nodes.constFind(id);
        if (it == d->nodes.constEnd())
            continue;
        QJsonObject node;
        node.insert(kId, id);
        node.insert(kParent, it.value().parent);
        node.insert(kMessage, it.value().message.toJson());
        nodes.append(node);
    }

    QJsonObject json;
    if (!d->systemPrompt.isEmpty())
        json.insert(kSystemPrompt, d->systemPrompt);
    json.insert(kNodes, nodes);
    json.insert(kActiveLeaf, d->activeLeaf);
    return json;
}

Transcript Transcript::fromJson(const QJsonObject &json)
{
    Transcript transcript;
    transcript.d->systemPrompt = json.value(kSystemPrompt).toString();

    const QJsonArray nodes = json.value(kNodes).toArray();
    for (const QJsonValue &value : nodes) {
        const QJsonObject object = value.toObject();
        const NodeId id = object.value(kId).toInt();
        if (id <= InvalidNode)
            continue;

        TranscriptData::Node node;
        node.parent = object.value(kParent).toInt();
        node.message = Core::Message::fromJson(object.value(kMessage).toObject());
        transcript.d->nodes.insert(id, node);

        // A parent written after its child, or missing entirely, leaves the
        // node a root rather than dangling.
        if (node.parent != InvalidNode && transcript.d->nodes.contains(node.parent))
            transcript.d->nodes[node.parent].children.append(id);
        else
            transcript.d->roots.append(id);

        transcript.d->nextId = qMax(transcript.d->nextId, id + 1);
    }

    const NodeId leaf = json.value(kActiveLeaf).toInt();
    transcript.d->activeLeaf = transcript.d->nodes.contains(leaf) ? leaf : InvalidNode;
    return transcript;
}

void Transcript::clear()
{
    d->nodes.clear();
    d->roots.clear();
    d->activeLeaf = InvalidNode;
    d->nextId = 1;
}

bool Transcript::operator==(const Transcript &other) const
{
    if (d->systemPrompt != other.d->systemPrompt || d->activeLeaf != other.d->activeLeaf
        || d->nodes.size() != other.d->nodes.size() || d->roots != other.d->roots) {
        return false;
    }
    for (auto it = d->nodes.constBegin(); it != d->nodes.constEnd(); ++it) {
        auto mine = it.value();
        auto theirs = other.d->nodes.constFind(it.key());
        if (theirs == other.d->nodes.constEnd() || theirs->parent != mine.parent
            || theirs->children != mine.children || !(theirs->message == mine.message)) {
            return false;
        }
    }
    return true;
}

} // namespace Chat
} // namespace QtOpenAi
