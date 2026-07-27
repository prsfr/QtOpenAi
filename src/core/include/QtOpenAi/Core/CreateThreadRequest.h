// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/Enums.h>
#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

// A message to put into a thread — when creating it (POST /threads), when
// appending to it (POST /threads/{id}/messages), or as an extra message on a run
// (`additional_messages`).
//
// The common case is a plain user turn, which only needs `text`. Multimodal
// input goes through `content` instead: an array of Assistants content parts
// (`{"type":"image_file", ...}`, ...), which wins over `text` when non-empty.
struct QTOPENAI_CORE_EXPORT ThreadMessageInput
{
    // "user" or "assistant"; the API rejects the other roles here.
    Role role = Role::User;
    // Plain-text content, sent as a bare `content` string.
    QString text;
    // Content parts, sent as a `content` array instead of `text`.
    QJsonArray content;
    // Files to attach and the tools that may use them.
    QJsonArray attachments;
    QJsonObject metadata;

    QJsonObject toJson() const;
    static ThreadMessageInput fromJson(const QJsonObject &json);

    bool operator==(const ThreadMessageInput &other) const
    {
        return role == other.role && text == other.text && content == other.content
               && attachments == other.attachments && metadata == other.metadata;
    }
    bool operator!=(const ThreadMessageInput &other) const { return !(*this == other); }
};

class CreateThreadRequestData;

// The body of a POST /threads request: a thread, optionally seeded with the
// messages it starts from. Every field is optional — an empty body creates an
// empty thread.
//
// The same object is nested under `thread` when creating a thread and running it
// in one call (POST /threads/runs), which CreateRunRequest::setThread() does.
class QTOPENAI_CORE_EXPORT CreateThreadRequest
{
public:
    CreateThreadRequest();
    CreateThreadRequest(const CreateThreadRequest &other);
    CreateThreadRequest(CreateThreadRequest &&other) noexcept;
    CreateThreadRequest &operator=(const CreateThreadRequest &other);
    CreateThreadRequest &operator=(CreateThreadRequest &&other) noexcept;
    ~CreateThreadRequest();

    void swap(CreateThreadRequest &other) noexcept { d.swap(other.d); }

    QList<ThreadMessageInput> messages() const;
    void setMessages(const QList<ThreadMessageInput> &messages);
    void addMessage(const ThreadMessageInput &message);
    // Convenience for the common case: one plain-text user turn.
    void addUserMessage(const QString &text);

    QJsonObject toolResources() const;
    void setToolResources(const QJsonObject &toolResources);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    // True when nothing was set, so the caller can leave the body out.
    bool isEmpty() const;

    QJsonObject toJson() const;

    bool operator==(const CreateThreadRequest &other) const;
    bool operator!=(const CreateThreadRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CreateThreadRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::CreateThreadRequest)
