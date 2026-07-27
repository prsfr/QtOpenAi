// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class ChatKitThreadItemData;

// One entry of a ChatKit thread (GET /chatkit/threads/{id}/items).
//
// The API returns a six-way discriminated union here: user and assistant
// messages, rendered widgets, client tool calls, tasks and task groups. They
// agree on an envelope — id, object, created_at, thread_id, type — and on
// nothing else, so only the envelope and the message `content` are typed. Every
// other field of every variant is preserved verbatim in raw() and written back
// unchanged, so an item this library does not model still survives a round trip
// instead of being silently dropped.
//
// text() reads the messages: both user content blocks (`input_text`,
// `quoted_text`) and assistant `output_text` segments carry their text flat, so
// one walk covers both directions of the conversation.
class QTOPENAI_CORE_EXPORT ChatKitThreadItem
{
public:
    ChatKitThreadItem();
    ChatKitThreadItem(const ChatKitThreadItem &other);
    ChatKitThreadItem(ChatKitThreadItem &&other) noexcept;
    ChatKitThreadItem &operator=(const ChatKitThreadItem &other);
    ChatKitThreadItem &operator=(ChatKitThreadItem &&other) noexcept;
    ~ChatKitThreadItem();

    void swap(ChatKitThreadItem &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "chatkit.thread_item" for every variant.
    QString object() const;
    void setObject(const QString &object);

    // Unix creation timestamp (`created_at`); 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    // The thread this item belongs to.
    QString threadId() const;
    void setThreadId(const QString &threadId);

    // The variant, e.g. "chatkit.user_message", "chatkit.assistant_message",
    // "chatkit.widget", "chatkit.client_tool_call", "chatkit.task".
    QString type() const;
    void setType(const QString &type);

    // The content blocks of a message variant; empty for the others.
    QJsonArray content() const;
    void setContent(const QJsonArray &content);

    // Every field outside the envelope and `content`, verbatim — the payload of
    // whichever variant this is.
    QJsonObject raw() const;
    void setRaw(const QJsonObject &raw);

    bool isUserMessage() const { return type() == QLatin1String("chatkit.user_message"); }
    bool isAssistantMessage() const { return type() == QLatin1String("chatkit.assistant_message"); }

    // The readable text of a message item; empty for the other variants.
    QString text() const;

    QJsonObject toJson() const;
    static ChatKitThreadItem fromJson(const QJsonObject &json);

    bool operator==(const ChatKitThreadItem &other) const;
    bool operator!=(const ChatKitThreadItem &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ChatKitThreadItemData> d;
};

// A `list` of thread items (GET /chatkit/threads/{id}/items). Cursor-paginated;
// reuses the shared list-page type.
using ChatKitThreadItemList = ListPage<ChatKitThreadItem>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ChatKitThreadItem)
Q_DECLARE_METATYPE(QtOpenAi::Core::ChatKitThreadItem)
Q_DECLARE_METATYPE(QtOpenAi::Core::ChatKitThreadItemList)
