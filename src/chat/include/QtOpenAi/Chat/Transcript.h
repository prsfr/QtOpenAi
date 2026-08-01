// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Chat/GlobalChat.h>
#include <QtOpenAi/Chat/TrimPolicy.h>
#include <QtOpenAi/Core/ChatCompletionRequest.h>
#include <QtOpenAi/Core/Message.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Chat {

class TranscriptData;

// The running conversation, held locally.
//
// Distinct from Core::Conversation, which is the *server-side* Conversations
// API resource: this one belongs to the client, works against any
// OpenAI-compatible endpoint, and is what you build the next request from.
//
//     Transcript transcript;
//     transcript.setSystemPrompt("You are terse.");
//     transcript.addUserMessage("Why is the sky blue?");
//     client.createChatCompletion(transcript.buildRequest("gpt-4o-mini"));
//     // ... and when the answer arrives:
//     transcript.addMessage(response.firstMessage());
//
// **It is a tree, and linear use is the tree that never branched.** Editing a
// past question does not overwrite it: `fork()` gives that message a sibling
// and makes the new one active, so both answers stay reachable -- the behaviour
// every chat UI with a "‹ 2/3 ›" control has. A transcript that is only ever
// appended to has one child per node and reads as an ordinary list, which is
// why there is one type here rather than two.
//
// `messages()` is the path from the root to the active leaf, with the system
// prompt in front and the trim policy applied. That path is the conversation as
// far as the model is concerned; everything else in the tree is history the
// application can navigate back to.
class QTOPENAI_CHAT_EXPORT Transcript
{
public:
    // Addresses a node for as long as this transcript lives, and across
    // toJson()/fromJson(). Zero is never a node.
    using NodeId = int;
    static constexpr NodeId InvalidNode = 0;

    Transcript();
    Transcript(const Transcript &other);
    Transcript(Transcript &&other) noexcept;
    Transcript &operator=(const Transcript &other);
    Transcript &operator=(Transcript &&other) noexcept;
    ~Transcript();

    void swap(Transcript &other) noexcept { d.swap(other.d); }

    // Kept outside the tree: it belongs to the conversation as a whole, not to
    // any turn in it, and it is never trimmed away.
    QString systemPrompt() const;
    void setSystemPrompt(const QString &prompt);

    // --- Appending --------------------------------------------------------
    // Each adds a child of the active leaf and makes it active.
    NodeId addMessage(const Core::Message &message);
    NodeId addUserMessage(const QString &text);
    QList<NodeId> addMessages(const QList<Core::Message> &messages);

    // --- Branching --------------------------------------------------------
    // Add a sibling of `node` -- the message it should have been -- and make it
    // active. The original branch stays reachable through siblings().
    NodeId fork(NodeId node, const Core::Message &message);

    NodeId activeLeaf() const;
    // Moves the active path to this node; anything below it is no longer part
    // of the context, but is still in the tree.
    bool setActiveLeaf(NodeId node);

    NodeId parent(NodeId node) const;
    QList<NodeId> children(NodeId node) const;
    // Every node with the same parent, in creation order, including `node`.
    QList<NodeId> siblings(NodeId node) const;
    QList<NodeId> roots() const;

    Core::Message message(NodeId node) const;
    bool setMessage(NodeId node, const Core::Message &message);
    bool contains(NodeId node) const;
    int count() const;
    bool isEmpty() const;

    // Root to active leaf, the linear conversation this transcript is
    // currently on.
    QList<NodeId> activePath() const;

    // --- The request ------------------------------------------------------
    TrimPolicy trimPolicy() const;
    void setTrimPolicy(const TrimPolicy &policy);

    // The system prompt, then the active path, then the trim policy.
    QList<Core::Message> messages() const;

    // The same messages as a request ready to send.
    Core::ChatCompletionRequest buildRequest(const QString &model) const;

    // --- Persistence ------------------------------------------------------
    // Node ids, the tree and the active leaf all survive; the trim policy does
    // not, being behaviour rather than history.
    QJsonObject toJson() const;
    static Transcript fromJson(const QJsonObject &json);

    void clear();

    bool operator==(const Transcript &other) const;
    bool operator!=(const Transcript &other) const { return !(*this == other); }

private:
    QSharedDataPointer<TranscriptData> d;
};

} // namespace Chat
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Chat::Transcript)
