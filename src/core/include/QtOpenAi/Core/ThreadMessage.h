// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/Enums.h>
#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class ThreadMessageData;

// One message in a thread (POST/GET /threads/{id}/messages, ...).
//
// The Assistants content parts are shaped differently from the chat ones this
// library models in ContentPart: a text part nests its value and annotations
// under `text`, and an image part names a file rather than a URL. The array is
// therefore carried verbatim, with text() pulling the readable part out — which
// is what a caller almost always wants:
//
//     for (const Core::ThreadMessage &message : list.data)
//         qDebug() << Core::roleToString(message.role()) << message.text();
class QTOPENAI_CORE_EXPORT ThreadMessage
{
public:
    ThreadMessage();
    ThreadMessage(const ThreadMessage &other);
    ThreadMessage(ThreadMessage &&other) noexcept;
    ThreadMessage &operator=(const ThreadMessage &other);
    ThreadMessage &operator=(ThreadMessage &&other) noexcept;
    ~ThreadMessage();

    void swap(ThreadMessage &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "thread.message" (or "thread.message.deleted").
    QString object() const;
    void setObject(const QString &object);

    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    QString threadId() const;
    void setThreadId(const QString &threadId);

    // "in_progress", "incomplete" or "completed". Kept as a string: it is a
    // per-message delivery state, unrelated to the run's own status set.
    QString status() const;
    void setStatus(const QString &status);

    // Why an incomplete message stopped (`incomplete_details`), verbatim.
    QJsonObject incompleteDetails() const;
    void setIncompleteDetails(const QJsonObject &incompleteDetails);

    qint64 completedAt() const;
    void setCompletedAt(qint64 completedAt);

    qint64 incompleteAt() const;
    void setIncompleteAt(qint64 incompleteAt);

    // "user" or "assistant".
    Role role() const;
    void setRole(Role role);

    // The content parts (`content`), verbatim. See text() for the readable part.
    QJsonArray content() const;
    void setContent(const QJsonArray &content);

    // The text of every `text` content part, concatenated; empty when the
    // message carries none.
    QString text() const;

    // Which assistant and run produced the message; both empty for a message the
    // caller added.
    QString assistantId() const;
    void setAssistantId(const QString &assistantId);

    QString runId() const;
    void setRunId(const QString &runId);

    // Files attached to the message and the tools they are meant for
    // (`attachments`), verbatim.
    QJsonArray attachments() const;
    void setAttachments(const QJsonArray &attachments);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    QJsonObject toJson() const;
    static ThreadMessage fromJson(const QJsonObject &json);

    bool operator==(const ThreadMessage &other) const;
    bool operator!=(const ThreadMessage &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ThreadMessageData> d;
};

// A `list` of thread messages (GET /threads/{id}/messages).
using ThreadMessageList = ListPage<ThreadMessage>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ThreadMessage)
Q_DECLARE_METATYPE(QtOpenAi::Core::ThreadMessage)
Q_DECLARE_METATYPE(QtOpenAi::Core::ThreadMessageList)
