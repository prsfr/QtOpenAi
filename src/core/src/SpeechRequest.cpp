// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/SpeechRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class SpeechRequestData : public QSharedData
{
public:
    QString model;
    QString input;
    QString voice;
    QString responseFormat;
    std::optional<double> speed;
    QString instructions;
    QString streamFormat;
};

SpeechRequest::SpeechRequest()
    : d(new SpeechRequestData)
{ }

SpeechRequest::SpeechRequest(QString model, QString input, QString voice)
    : d(new SpeechRequestData)
{
    d->model = std::move(model);
    d->input = std::move(input);
    d->voice = std::move(voice);
}

SpeechRequest::SpeechRequest(const SpeechRequest &other) = default;
SpeechRequest::SpeechRequest(SpeechRequest &&other) noexcept = default;
SpeechRequest &SpeechRequest::operator=(const SpeechRequest &other) = default;
SpeechRequest &SpeechRequest::operator=(SpeechRequest &&other) noexcept = default;
SpeechRequest::~SpeechRequest() = default;

QString SpeechRequest::model() const { return d->model; }
void SpeechRequest::setModel(const QString &model) { d->model = model; }

QString SpeechRequest::input() const { return d->input; }
void SpeechRequest::setInput(const QString &input) { d->input = input; }

QString SpeechRequest::voice() const { return d->voice; }
void SpeechRequest::setVoice(const QString &voice) { d->voice = voice; }

QString SpeechRequest::responseFormat() const { return d->responseFormat; }
void SpeechRequest::setResponseFormat(const QString &format) { d->responseFormat = format; }

std::optional<double> SpeechRequest::speed() const { return d->speed; }
void SpeechRequest::setSpeed(double speed) { d->speed = speed; }

QString SpeechRequest::instructions() const { return d->instructions; }
void SpeechRequest::setInstructions(const QString &instructions) { d->instructions = instructions; }

QString SpeechRequest::streamFormat() const { return d->streamFormat; }
void SpeechRequest::setStreamFormat(const QString &streamFormat) { d->streamFormat = streamFormat; }

QJsonObject SpeechRequest::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("model"), d->model);
    json.insert(QStringLiteral("input"), d->input);
    json.insert(QStringLiteral("voice"), d->voice);
    detail::insertIfNotEmpty(json, QStringLiteral("response_format"), d->responseFormat);
    if (d->speed)
        json.insert(QStringLiteral("speed"), *d->speed);
    detail::insertIfNotEmpty(json, QStringLiteral("instructions"), d->instructions);
    detail::insertIfNotEmpty(json, QStringLiteral("stream_format"), d->streamFormat);
    return json;
}

SpeechRequest SpeechRequest::fromJson(const QJsonObject &json)
{
    SpeechRequest request;
    request.d->model = detail::stringOr(json, QStringLiteral("model"));
    request.d->input = detail::stringOr(json, QStringLiteral("input"));
    request.d->voice = detail::stringOr(json, QStringLiteral("voice"));
    request.d->responseFormat = detail::stringOr(json, QStringLiteral("response_format"));
    if (json.contains(QStringLiteral("speed")))
        request.d->speed = json.value(QStringLiteral("speed")).toDouble();
    request.d->instructions = detail::stringOr(json, QStringLiteral("instructions"));
    request.d->streamFormat = detail::stringOr(json, QStringLiteral("stream_format"));
    return request;
}

bool SpeechRequest::operator==(const SpeechRequest &other) const
{
    return d->model == other.d->model && d->input == other.d->input && d->voice == other.d->voice
           && d->responseFormat == other.d->responseFormat && d->speed == other.d->speed
           && d->instructions == other.d->instructions && d->streamFormat == other.d->streamFormat;
}

} // namespace Core
} // namespace QtOpenAi
