// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ResponseUsage.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ResponseUsageData : public QSharedData
{
public:
    int inputTokens = 0;
    int outputTokens = 0;
    int totalTokens = 0;
    int reasoningTokens = 0;
};

ResponseUsage::ResponseUsage()
    : d(new ResponseUsageData)
{ }

ResponseUsage::ResponseUsage(const ResponseUsage &other) = default;
ResponseUsage::ResponseUsage(ResponseUsage &&other) noexcept = default;
ResponseUsage &ResponseUsage::operator=(const ResponseUsage &other) = default;
ResponseUsage &ResponseUsage::operator=(ResponseUsage &&other) noexcept = default;
ResponseUsage::~ResponseUsage() = default;

int ResponseUsage::inputTokens() const { return d->inputTokens; }
void ResponseUsage::setInputTokens(int tokens) { d->inputTokens = tokens; }

int ResponseUsage::outputTokens() const { return d->outputTokens; }
void ResponseUsage::setOutputTokens(int tokens) { d->outputTokens = tokens; }

int ResponseUsage::totalTokens() const { return d->totalTokens; }
void ResponseUsage::setTotalTokens(int tokens) { d->totalTokens = tokens; }

int ResponseUsage::reasoningTokens() const { return d->reasoningTokens; }
void ResponseUsage::setReasoningTokens(int tokens) { d->reasoningTokens = tokens; }

QJsonObject ResponseUsage::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("input_tokens"), d->inputTokens);
    json.insert(QStringLiteral("output_tokens"), d->outputTokens);
    json.insert(QStringLiteral("total_tokens"), d->totalTokens);

    QJsonObject outputDetails;
    outputDetails.insert(QStringLiteral("reasoning_tokens"), d->reasoningTokens);
    json.insert(QStringLiteral("output_tokens_details"), outputDetails);
    return json;
}

ResponseUsage ResponseUsage::fromJson(const QJsonObject &json)
{
    ResponseUsage usage;
    usage.d->inputTokens = json.value(QStringLiteral("input_tokens")).toInt();
    usage.d->outputTokens = json.value(QStringLiteral("output_tokens")).toInt();
    usage.d->totalTokens = json.value(QStringLiteral("total_tokens")).toInt();
    const QJsonObject outputDetails
            = json.value(QStringLiteral("output_tokens_details")).toObject();
    usage.d->reasoningTokens = outputDetails.value(QStringLiteral("reasoning_tokens")).toInt();
    return usage;
}

bool ResponseUsage::operator==(const ResponseUsage &other) const
{
    return d->inputTokens == other.d->inputTokens && d->outputTokens == other.d->outputTokens
           && d->totalTokens == other.d->totalTokens
           && d->reasoningTokens == other.d->reasoningTokens;
}

} // namespace Core
} // namespace QtOpenAi
