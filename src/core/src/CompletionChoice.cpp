// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/CompletionChoice.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class CompletionChoiceData : public QSharedData
{
public:
    QString text;
    int index = 0;
    QString finishReason;
    QJsonValue logprobs = QJsonValue::Null;
};

CompletionChoice::CompletionChoice()
    : d(new CompletionChoiceData)
{ }

CompletionChoice::CompletionChoice(const CompletionChoice &other) = default;
CompletionChoice::CompletionChoice(CompletionChoice &&other) noexcept = default;
CompletionChoice &CompletionChoice::operator=(const CompletionChoice &other) = default;
CompletionChoice &CompletionChoice::operator=(CompletionChoice &&other) noexcept = default;
CompletionChoice::~CompletionChoice() = default;

QString CompletionChoice::text() const { return d->text; }
void CompletionChoice::setText(const QString &text) { d->text = text; }

int CompletionChoice::index() const { return d->index; }
void CompletionChoice::setIndex(int index) { d->index = index; }

QString CompletionChoice::finishReason() const { return d->finishReason; }
void CompletionChoice::setFinishReason(const QString &finishReason)
{
    d->finishReason = finishReason;
}

QJsonValue CompletionChoice::logprobs() const { return d->logprobs; }
void CompletionChoice::setLogprobs(const QJsonValue &logprobs) { d->logprobs = logprobs; }

QJsonObject CompletionChoice::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("text"), d->text);
    json.insert(QStringLiteral("index"), d->index);
    json.insert(QStringLiteral("logprobs"), d->logprobs);
    if (!d->finishReason.isEmpty())
        json.insert(QStringLiteral("finish_reason"), d->finishReason);
    else
        json.insert(QStringLiteral("finish_reason"), QJsonValue::Null);
    return json;
}

CompletionChoice CompletionChoice::fromJson(const QJsonObject &json)
{
    CompletionChoice choice;
    choice.d->text = detail::stringOr(json, QStringLiteral("text"));
    choice.d->index = json.value(QStringLiteral("index")).toInt();
    choice.d->finishReason = detail::stringOr(json, QStringLiteral("finish_reason"));
    choice.d->logprobs = json.value(QStringLiteral("logprobs"));
    return choice;
}

bool CompletionChoice::operator==(const CompletionChoice &other) const
{
    return d->text == other.d->text && d->index == other.d->index
           && d->finishReason == other.d->finishReason && d->logprobs == other.d->logprobs;
}

} // namespace Core
} // namespace QtOpenAi
