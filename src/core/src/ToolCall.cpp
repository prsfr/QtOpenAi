// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ToolCall.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ToolCallData : public QSharedData
{
public:
    QString id;
    QString type = QStringLiteral("function");
    FunctionCall function;
};

ToolCall::ToolCall()
    : d(new ToolCallData)
{ }

ToolCall::ToolCall(QString id, FunctionCall function)
    : d(new ToolCallData)
{
    d->id = std::move(id);
    d->function = std::move(function);
}

ToolCall::ToolCall(const ToolCall &other) = default;
ToolCall::ToolCall(ToolCall &&other) noexcept = default;
ToolCall &ToolCall::operator=(const ToolCall &other) = default;
ToolCall &ToolCall::operator=(ToolCall &&other) noexcept = default;
ToolCall::~ToolCall() = default;

QString ToolCall::id() const { return d->id; }
void ToolCall::setId(const QString &id) { d->id = id; }

QString ToolCall::type() const { return d->type; }
void ToolCall::setType(const QString &type) { d->type = type; }

FunctionCall ToolCall::function() const { return d->function; }
void ToolCall::setFunction(const FunctionCall &function) { d->function = function; }

bool ToolCall::isEmpty() const { return d->id.isEmpty() && d->function.isEmpty(); }

QJsonObject ToolCall::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("id"), d->id);
    json.insert(QStringLiteral("type"), d->type);
    json.insert(QStringLiteral("function"), d->function.toJson());
    return json;
}

ToolCall ToolCall::fromJson(const QJsonObject &json)
{
    ToolCall call;
    call.d->id = detail::stringOr(json, QStringLiteral("id"));
    call.d->type = detail::stringOr(json, QStringLiteral("type"), QStringLiteral("function"));
    call.d->function = FunctionCall::fromJson(json.value(QStringLiteral("function")).toObject());
    return call;
}

bool ToolCall::operator==(const ToolCall &other) const
{
    return d->id == other.d->id && d->type == other.d->type && d->function == other.d->function;
}

} // namespace Core
} // namespace QtOpenAi
