// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/Enums.h>
#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class ChatKitThreadData;

// A ChatKit thread (GET/DELETE /chatkit/threads/{id}, GET /chatkit/threads) —
// one conversation held by the hosted UI, owned by the end user the session was
// scoped to.
//
// Threads are created by the ChatKit frontend as the user talks, not through
// this API: the REST surface only lists, reads and deletes them, which is why
// there is no create request type here.
//
// `status` is the one status in the library that arrives as a tagged object
// ({"type": "locked", "reason": ...}) rather than a bare string, so it is split
// into the enum plus statusReason() and reassembled on the way out.
//
// The deletion acknowledgement of DELETE /chatkit/threads/{id} also decodes into
// this type, reporting the object as "chatkit.thread.deleted".
class QTOPENAI_CORE_EXPORT ChatKitThread
{
public:
    ChatKitThread();
    ChatKitThread(const ChatKitThread &other);
    ChatKitThread(ChatKitThread &&other) noexcept;
    ChatKitThread &operator=(const ChatKitThread &other);
    ChatKitThread &operator=(ChatKitThread &&other) noexcept;
    ~ChatKitThread();

    void swap(ChatKitThread &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "chatkit.thread" (or "chatkit.thread.deleted").
    QString object() const;
    void setObject(const QString &object);

    // Unix creation timestamp (`created_at`); 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    // The generated title, empty until ChatKit has written one.
    QString title() const;
    void setTitle(const QString &title);

    ChatKitThreadStatus status() const;
    void setStatus(ChatKitThreadStatus status);

    // Why the thread was locked or closed; empty when none was recorded, and
    // never set for an active thread.
    QString statusReason() const;
    void setStatusReason(const QString &statusReason);

    // The end user who owns the thread.
    QString user() const;
    void setUser(const QString &user);

    QJsonObject toJson() const;
    static ChatKitThread fromJson(const QJsonObject &json);

    bool operator==(const ChatKitThread &other) const;
    bool operator!=(const ChatKitThread &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ChatKitThreadData> d;
};

// A `list` of ChatKit threads (GET /chatkit/threads). Cursor-paginated; reuses
// the shared list-page type.
using ChatKitThreadList = ListPage<ChatKitThread>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ChatKitThread)
Q_DECLARE_METATYPE(QtOpenAi::Core::ChatKitThread)
Q_DECLARE_METATYPE(QtOpenAi::Core::ChatKitThreadList)
