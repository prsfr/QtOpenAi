// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class ThreadData;

// A conversation thread (POST /threads, GET/POST/DELETE /threads/{id}).
//
// A thread is a server-side container of messages that an assistant is run
// against; unlike a chat completion it keeps the transcript for you, so a run
// only needs the thread id. The thread object itself carries almost nothing —
// its messages and runs are separate resources (ThreadMessage, Run).
//
// The deletion acknowledgement of DELETE /threads/{id} also decodes into this
// type; it reports the object as "thread.deleted".
class QTOPENAI_CORE_EXPORT Thread
{
public:
    Thread();
    Thread(const Thread &other);
    Thread(Thread &&other) noexcept;
    Thread &operator=(const Thread &other);
    Thread &operator=(Thread &&other) noexcept;
    ~Thread();

    void swap(Thread &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "thread" (or "thread.deleted").
    QString object() const;
    void setObject(const QString &object);

    // Unix creation timestamp (`created_at`); 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    // Resources the assistant's tools may use in this thread
    // (`tool_resources`), verbatim — an open union like the assistant's own.
    QJsonObject toolResources() const;
    void setToolResources(const QJsonObject &toolResources);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    QJsonObject toJson() const;
    static Thread fromJson(const QJsonObject &json);

    bool operator==(const Thread &other) const;
    bool operator!=(const Thread &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ThreadData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Thread)
Q_DECLARE_METATYPE(QtOpenAi::Core::Thread)
