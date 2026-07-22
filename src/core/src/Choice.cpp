// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Choice.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ChoiceData : public QSharedData
{
public:
    int index = 0;
    Message message;
    FinishReason finishReason = FinishReason::None;
};

Choice::Choice()
    : d(new ChoiceData)
{
}

Choice::Choice(const Choice &other) = default;
Choice::Choice(Choice &&other) noexcept = default;
Choice &Choice::operator=(const Choice &other) = default;
Choice &Choice::operator=(Choice &&other) noexcept = default;
Choice::~Choice() = default;

int Choice::index() const { return d->index; }
void Choice::setIndex(int index) { d->index = index; }

Message Choice::message() const { return d->message; }
void Choice::setMessage(const Message &message) { d->message = message; }

FinishReason Choice::finishReason() const { return d->finishReason; }
void Choice::setFinishReason(FinishReason reason) { d->finishReason = reason; }

QJsonObject Choice::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("index"), d->index);
    json.insert(QStringLiteral("message"), d->message.toJson());
    const QString reason = finishReasonToString(d->finishReason);
    json.insert(QStringLiteral("finish_reason"),
                reason.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(reason));
    return json;
}

Choice Choice::fromJson(const QJsonObject &json)
{
    Choice choice;
    choice.d->index = json.value(QStringLiteral("index")).toInt();
    choice.d->message = Message::fromJson(json.value(QStringLiteral("message")).toObject());
    choice.d->finishReason = finishReasonFromString(detail::stringOr(json, QStringLiteral("finish_reason")));
    return choice;
}

bool Choice::operator==(const Choice &other) const
{
    return d->index == other.d->index
        && d->message == other.d->message
        && d->finishReason == other.d->finishReason;
}

} // namespace Core
} // namespace QtOpenAi
