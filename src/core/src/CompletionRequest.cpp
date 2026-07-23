// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/CompletionRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class CompletionRequestData : public QSharedData
{
public:
    QString model;
    QJsonValue prompt;
    std::optional<int> maxTokens;
    std::optional<double> temperature;
    std::optional<double> topP;
    std::optional<int> n;
    std::optional<bool> echo;
    std::optional<QJsonValue> stop;
    std::optional<double> presencePenalty;
    std::optional<double> frequencyPenalty;
    std::optional<int> bestOf;
    std::optional<int> seed;
    QString suffix;
    std::optional<bool> stream;
    QJsonObject extraBody;
};

CompletionRequest::CompletionRequest()
    : d(new CompletionRequestData)
{ }

CompletionRequest::CompletionRequest(QString model, QString prompt)
    : d(new CompletionRequestData)
{
    d->model = std::move(model);
    d->prompt = std::move(prompt);
}

CompletionRequest::CompletionRequest(const CompletionRequest &other) = default;
CompletionRequest::CompletionRequest(CompletionRequest &&other) noexcept = default;
CompletionRequest &CompletionRequest::operator=(const CompletionRequest &other) = default;
CompletionRequest &CompletionRequest::operator=(CompletionRequest &&other) noexcept = default;
CompletionRequest::~CompletionRequest() = default;

QString CompletionRequest::model() const { return d->model; }
void CompletionRequest::setModel(const QString &model) { d->model = model; }

QJsonValue CompletionRequest::prompt() const { return d->prompt; }
void CompletionRequest::setPrompt(const QJsonValue &prompt) { d->prompt = prompt; }
void CompletionRequest::setPrompt(const QString &prompt) { d->prompt = prompt; }

std::optional<int> CompletionRequest::maxTokens() const { return d->maxTokens; }
void CompletionRequest::setMaxTokens(int maxTokens) { d->maxTokens = maxTokens; }

std::optional<double> CompletionRequest::temperature() const { return d->temperature; }
void CompletionRequest::setTemperature(double temperature) { d->temperature = temperature; }

std::optional<double> CompletionRequest::topP() const { return d->topP; }
void CompletionRequest::setTopP(double topP) { d->topP = topP; }

std::optional<int> CompletionRequest::n() const { return d->n; }
void CompletionRequest::setN(int n) { d->n = n; }

std::optional<bool> CompletionRequest::echo() const { return d->echo; }
void CompletionRequest::setEcho(bool echo) { d->echo = echo; }

std::optional<QJsonValue> CompletionRequest::stop() const { return d->stop; }
void CompletionRequest::setStop(const QJsonValue &stop) { d->stop = stop; }

std::optional<double> CompletionRequest::presencePenalty() const { return d->presencePenalty; }
void CompletionRequest::setPresencePenalty(double penalty) { d->presencePenalty = penalty; }

std::optional<double> CompletionRequest::frequencyPenalty() const { return d->frequencyPenalty; }
void CompletionRequest::setFrequencyPenalty(double penalty) { d->frequencyPenalty = penalty; }

std::optional<int> CompletionRequest::bestOf() const { return d->bestOf; }
void CompletionRequest::setBestOf(int bestOf) { d->bestOf = bestOf; }

std::optional<int> CompletionRequest::seed() const { return d->seed; }
void CompletionRequest::setSeed(int seed) { d->seed = seed; }

QString CompletionRequest::suffix() const { return d->suffix; }
void CompletionRequest::setSuffix(const QString &suffix) { d->suffix = suffix; }

std::optional<bool> CompletionRequest::stream() const { return d->stream; }
void CompletionRequest::setStream(bool stream) { d->stream = stream; }

QJsonObject CompletionRequest::extraBody() const { return d->extraBody; }
void CompletionRequest::setExtraBody(const QJsonObject &extra) { d->extraBody = extra; }

QJsonObject CompletionRequest::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("model"), d->model);
    if (!d->prompt.isNull() && !d->prompt.isUndefined())
        json.insert(QStringLiteral("prompt"), d->prompt);

    if (d->maxTokens)
        json.insert(QStringLiteral("max_tokens"), *d->maxTokens);
    if (d->temperature)
        json.insert(QStringLiteral("temperature"), *d->temperature);
    if (d->topP)
        json.insert(QStringLiteral("top_p"), *d->topP);
    if (d->n)
        json.insert(QStringLiteral("n"), *d->n);
    if (d->echo)
        json.insert(QStringLiteral("echo"), *d->echo);
    if (d->stop)
        json.insert(QStringLiteral("stop"), *d->stop);
    if (d->presencePenalty)
        json.insert(QStringLiteral("presence_penalty"), *d->presencePenalty);
    if (d->frequencyPenalty)
        json.insert(QStringLiteral("frequency_penalty"), *d->frequencyPenalty);
    if (d->bestOf)
        json.insert(QStringLiteral("best_of"), *d->bestOf);
    if (d->seed)
        json.insert(QStringLiteral("seed"), *d->seed);
    detail::insertIfNotEmpty(json, QStringLiteral("suffix"), d->suffix);
    if (d->stream)
        json.insert(QStringLiteral("stream"), *d->stream);

    for (auto it = d->extraBody.constBegin(); it != d->extraBody.constEnd(); ++it) {
        if (!json.contains(it.key()))
            json.insert(it.key(), it.value());
    }
    return json;
}

CompletionRequest CompletionRequest::fromJson(const QJsonObject &json)
{
    CompletionRequest request;
    request.d->model = detail::stringOr(json, QStringLiteral("model"));
    request.d->prompt = json.value(QStringLiteral("prompt"));
    if (json.contains(QStringLiteral("max_tokens")))
        request.d->maxTokens = json.value(QStringLiteral("max_tokens")).toInt();
    if (json.contains(QStringLiteral("temperature")))
        request.d->temperature = json.value(QStringLiteral("temperature")).toDouble();
    if (json.contains(QStringLiteral("top_p")))
        request.d->topP = json.value(QStringLiteral("top_p")).toDouble();
    if (json.contains(QStringLiteral("n")))
        request.d->n = json.value(QStringLiteral("n")).toInt();
    if (json.contains(QStringLiteral("echo")))
        request.d->echo = json.value(QStringLiteral("echo")).toBool();
    if (json.contains(QStringLiteral("stop")))
        request.d->stop = json.value(QStringLiteral("stop"));
    if (json.contains(QStringLiteral("presence_penalty")))
        request.d->presencePenalty = json.value(QStringLiteral("presence_penalty")).toDouble();
    if (json.contains(QStringLiteral("frequency_penalty")))
        request.d->frequencyPenalty = json.value(QStringLiteral("frequency_penalty")).toDouble();
    if (json.contains(QStringLiteral("best_of")))
        request.d->bestOf = json.value(QStringLiteral("best_of")).toInt();
    if (json.contains(QStringLiteral("seed")))
        request.d->seed = json.value(QStringLiteral("seed")).toInt();
    request.d->suffix = detail::stringOr(json, QStringLiteral("suffix"));
    if (json.contains(QStringLiteral("stream")))
        request.d->stream = json.value(QStringLiteral("stream")).toBool();
    return request;
}

bool CompletionRequest::operator==(const CompletionRequest &other) const
{
    return d->model == other.d->model && d->prompt == other.d->prompt
           && d->maxTokens == other.d->maxTokens && d->temperature == other.d->temperature
           && d->topP == other.d->topP && d->n == other.d->n && d->echo == other.d->echo
           && d->stop == other.d->stop && d->presencePenalty == other.d->presencePenalty
           && d->frequencyPenalty == other.d->frequencyPenalty && d->bestOf == other.d->bestOf
           && d->seed == other.d->seed && d->suffix == other.d->suffix
           && d->stream == other.d->stream && d->extraBody == other.d->extraBody;
}

} // namespace Core
} // namespace QtOpenAi
