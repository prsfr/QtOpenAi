// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/Enums.h>
#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/Message.h>

#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>

namespace QtOpenAi {
namespace Core {

class ChoiceData;

// One completion choice: an index, the produced message, and a finish reason.
class QTOPENAI_CORE_EXPORT Choice
{
public:
    Choice();
    Choice(const Choice &other);
    Choice(Choice &&other) noexcept;
    Choice &operator=(const Choice &other);
    Choice &operator=(Choice &&other) noexcept;
    ~Choice();

    void swap(Choice &other) noexcept { d.swap(other.d); }

    int index() const;
    void setIndex(int index);

    Message message() const;
    void setMessage(const Message &message);

    FinishReason finishReason() const;
    void setFinishReason(FinishReason reason);

    QJsonObject toJson() const;
    static Choice fromJson(const QJsonObject &json);

    bool operator==(const Choice &other) const;
    bool operator!=(const Choice &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ChoiceData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Choice)
