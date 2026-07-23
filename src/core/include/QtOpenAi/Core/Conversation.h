// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class ConversationData;

// A stateful `conversation` object (Conversations API) that persists message
// and item history server-side for use with the Responses API.
class QTOPENAI_CORE_EXPORT Conversation
{
public:
    Conversation();
    Conversation(const Conversation &other);
    Conversation(Conversation &&other) noexcept;
    Conversation &operator=(const Conversation &other);
    Conversation &operator=(Conversation &&other) noexcept;
    ~Conversation();

    void swap(Conversation &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    QString object() const;
    void setObject(const QString &object);

    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    QJsonObject toJson() const;
    static Conversation fromJson(const QJsonObject &json);

    bool operator==(const Conversation &other) const;
    bool operator!=(const Conversation &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ConversationData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Conversation)
