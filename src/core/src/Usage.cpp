// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Usage.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class UsageData : public QSharedData
{
public:
    int promptTokens = 0;
    int completionTokens = 0;
    int totalTokens = 0;
};

Usage::Usage()
    : d(new UsageData)
{ }

Usage::Usage(const Usage &other) = default;
Usage::Usage(Usage &&other) noexcept = default;
Usage &Usage::operator=(const Usage &other) = default;
Usage &Usage::operator=(Usage &&other) noexcept = default;
Usage::~Usage() = default;

int Usage::promptTokens() const { return d->promptTokens; }
void Usage::setPromptTokens(int tokens) { d->promptTokens = tokens; }

int Usage::completionTokens() const { return d->completionTokens; }
void Usage::setCompletionTokens(int tokens) { d->completionTokens = tokens; }

int Usage::totalTokens() const { return d->totalTokens; }
void Usage::setTotalTokens(int tokens) { d->totalTokens = tokens; }

QJsonObject Usage::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("prompt_tokens"), d->promptTokens);
    json.insert(QStringLiteral("completion_tokens"), d->completionTokens);
    json.insert(QStringLiteral("total_tokens"), d->totalTokens);
    return json;
}

Usage Usage::fromJson(const QJsonObject &json)
{
    Usage usage;
    usage.d->promptTokens = json.value(QStringLiteral("prompt_tokens")).toInt();
    usage.d->completionTokens = json.value(QStringLiteral("completion_tokens")).toInt();
    usage.d->totalTokens = json.value(QStringLiteral("total_tokens")).toInt();
    return usage;
}

bool Usage::operator==(const Usage &other) const
{
    return d->promptTokens == other.d->promptTokens
           && d->completionTokens == other.d->completionTokens
           && d->totalTokens == other.d->totalTokens;
}

} // namespace Core
} // namespace QtOpenAi
