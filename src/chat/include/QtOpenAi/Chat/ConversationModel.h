// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Chat/GlobalChat.h>
#include <QtOpenAi/Chat/Transcript.h>

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

namespace QtOpenAi {
namespace Chat {

class ConversationModelPrivate;

// A Transcript that tells you when it changes.
//
// The value type has no way to announce anything, which is fine for building a
// request and useless for driving a view. This wraps one and emits what a UI
// needs to react to: a message arrived, the active branch moved, the whole
// thing was replaced.
//
//     connect(model, &ConversationModel::messageAdded, view, &View::appendRow);
//     connect(model, &ConversationModel::activeBranchChanged, view, &View::reload);
//
// It is a thin wrapper on purpose. The transcript remains the data -- take a
// copy with `transcript()`, persist it, hand it to another thread -- and this
// object is only the part that has to live where the signals are. A
// QAbstractItemModel over the same tree would sit on top of this rather than
// replacing it.
class QTOPENAI_CHAT_EXPORT ConversationModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(
            QString systemPrompt READ systemPrompt WRITE setSystemPrompt NOTIFY systemPromptChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    explicit ConversationModel(QObject *parent = nullptr);
    ~ConversationModel() override;

    // A copy, as with any value type -- mutating it does not touch the model.
    Transcript transcript() const;
    void setTranscript(const Transcript &transcript);

    QString systemPrompt() const;
    void setSystemPrompt(const QString &prompt);

    TrimPolicy trimPolicy() const;
    void setTrimPolicy(const TrimPolicy &policy);

    int count() const;

    Transcript::NodeId activeLeaf() const;
    Core::Message message(Transcript::NodeId node) const;
    QList<Transcript::NodeId> children(Transcript::NodeId node) const;
    QList<Transcript::NodeId> siblings(Transcript::NodeId node) const;
    QList<Transcript::NodeId> activePath() const;

    // The context as it would be sent, trim policy applied.
    QList<Core::Message> messages() const;
    Core::ChatCompletionRequest buildRequest(const QString &model) const;

public Q_SLOTS:
    QtOpenAi::Chat::Transcript::NodeId addUserMessage(const QString &text);
    QtOpenAi::Chat::Transcript::NodeId addMessage(const QtOpenAi::Core::Message &message);
    // Replace a past message with a different one, keeping the old branch.
    QtOpenAi::Chat::Transcript::NodeId fork(QtOpenAi::Chat::Transcript::NodeId node,
                                            const QtOpenAi::Core::Message &message);
    bool setActiveLeaf(QtOpenAi::Chat::Transcript::NodeId node);
    void clear();

Q_SIGNALS:
    // A node was added; `node` is it, `parent` is where it hangs.
    void messageAdded(int node, int parent);
    // The path root→active leaf is now a different one, which is what a view
    // showing the conversation has to redraw.
    void activeBranchChanged(int activeLeaf);
    void systemPromptChanged(const QString &prompt);
    void countChanged(int count);
    // The transcript was replaced or cleared: nothing incremental survives.
    void reset();

private:
    Q_DECLARE_PRIVATE(ConversationModel)
    QScopedPointer<ConversationModelPrivate> d_ptr;
};

} // namespace Chat
} // namespace QtOpenAi
