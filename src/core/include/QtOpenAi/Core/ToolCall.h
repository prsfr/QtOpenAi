// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/FunctionCall.h>
#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class ToolCallData;

// A single tool call produced by the model (currently always of type
// "function"). Implicitly shared value type with a hidden d-pointer.
class QTOPENAI_CORE_EXPORT ToolCall
{
public:
    ToolCall();
    explicit ToolCall(QString id, FunctionCall function);
    ToolCall(const ToolCall &other);
    ToolCall(ToolCall &&other) noexcept;
    ToolCall &operator=(const ToolCall &other);
    ToolCall &operator=(ToolCall &&other) noexcept;
    ~ToolCall();

    void swap(ToolCall &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The tool type. Defaults to "function" (the only value OpenAI supports).
    QString type() const;
    void setType(const QString &type);

    FunctionCall function() const;
    void setFunction(const FunctionCall &function);

    bool isEmpty() const;

    QJsonObject toJson() const;
    static ToolCall fromJson(const QJsonObject &json);

    bool operator==(const ToolCall &other) const;
    bool operator!=(const ToolCall &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ToolCallData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ToolCall)
