// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ImageGenerationRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ImageGenerationRequestData : public QSharedData
{
public:
    QString prompt;
    QString model;
    std::optional<int> n;
    QString size;
    QString quality;
    QString responseFormat;
    QString style;
    QString background;
    QString outputFormat;
    QString moderation;
    QString user;
    QJsonObject extraBody;
};

ImageGenerationRequest::ImageGenerationRequest()
    : d(new ImageGenerationRequestData)
{ }

ImageGenerationRequest::ImageGenerationRequest(QString prompt, QString model)
    : d(new ImageGenerationRequestData)
{
    d->prompt = std::move(prompt);
    d->model = std::move(model);
}

ImageGenerationRequest::ImageGenerationRequest(const ImageGenerationRequest &other) = default;
ImageGenerationRequest::ImageGenerationRequest(ImageGenerationRequest &&other) noexcept = default;
ImageGenerationRequest &ImageGenerationRequest::operator=(const ImageGenerationRequest &other)
        = default;
ImageGenerationRequest &ImageGenerationRequest::operator=(ImageGenerationRequest &&other) noexcept
        = default;
ImageGenerationRequest::~ImageGenerationRequest() = default;

QString ImageGenerationRequest::prompt() const { return d->prompt; }
void ImageGenerationRequest::setPrompt(const QString &prompt) { d->prompt = prompt; }

QString ImageGenerationRequest::model() const { return d->model; }
void ImageGenerationRequest::setModel(const QString &model) { d->model = model; }

std::optional<int> ImageGenerationRequest::n() const { return d->n; }
void ImageGenerationRequest::setN(int n) { d->n = n; }

QString ImageGenerationRequest::size() const { return d->size; }
void ImageGenerationRequest::setSize(const QString &size) { d->size = size; }

QString ImageGenerationRequest::quality() const { return d->quality; }
void ImageGenerationRequest::setQuality(const QString &quality) { d->quality = quality; }

QString ImageGenerationRequest::responseFormat() const { return d->responseFormat; }
void ImageGenerationRequest::setResponseFormat(const QString &format)
{
    d->responseFormat = format;
}

QString ImageGenerationRequest::style() const { return d->style; }
void ImageGenerationRequest::setStyle(const QString &style) { d->style = style; }

QString ImageGenerationRequest::background() const { return d->background; }
void ImageGenerationRequest::setBackground(const QString &background)
{
    d->background = background;
}

QString ImageGenerationRequest::outputFormat() const { return d->outputFormat; }
void ImageGenerationRequest::setOutputFormat(const QString &outputFormat)
{
    d->outputFormat = outputFormat;
}

QString ImageGenerationRequest::moderation() const { return d->moderation; }
void ImageGenerationRequest::setModeration(const QString &moderation)
{
    d->moderation = moderation;
}

QString ImageGenerationRequest::user() const { return d->user; }
void ImageGenerationRequest::setUser(const QString &user) { d->user = user; }

QJsonObject ImageGenerationRequest::extraBody() const { return d->extraBody; }
void ImageGenerationRequest::setExtraBody(const QJsonObject &extra) { d->extraBody = extra; }

QJsonObject ImageGenerationRequest::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("prompt"), d->prompt);
    detail::insertIfNotEmpty(json, QStringLiteral("model"), d->model);
    if (d->n)
        json.insert(QStringLiteral("n"), *d->n);
    detail::insertIfNotEmpty(json, QStringLiteral("size"), d->size);
    detail::insertIfNotEmpty(json, QStringLiteral("quality"), d->quality);
    detail::insertIfNotEmpty(json, QStringLiteral("response_format"), d->responseFormat);
    detail::insertIfNotEmpty(json, QStringLiteral("style"), d->style);
    detail::insertIfNotEmpty(json, QStringLiteral("background"), d->background);
    detail::insertIfNotEmpty(json, QStringLiteral("output_format"), d->outputFormat);
    detail::insertIfNotEmpty(json, QStringLiteral("moderation"), d->moderation);
    detail::insertIfNotEmpty(json, QStringLiteral("user"), d->user);

    for (auto it = d->extraBody.constBegin(); it != d->extraBody.constEnd(); ++it) {
        if (!json.contains(it.key()))
            json.insert(it.key(), it.value());
    }
    return json;
}

ImageGenerationRequest ImageGenerationRequest::fromJson(const QJsonObject &json)
{
    ImageGenerationRequest request;
    request.d->prompt = detail::stringOr(json, QStringLiteral("prompt"));
    request.d->model = detail::stringOr(json, QStringLiteral("model"));
    if (json.contains(QStringLiteral("n")))
        request.d->n = json.value(QStringLiteral("n")).toInt();
    request.d->size = detail::stringOr(json, QStringLiteral("size"));
    request.d->quality = detail::stringOr(json, QStringLiteral("quality"));
    request.d->responseFormat = detail::stringOr(json, QStringLiteral("response_format"));
    request.d->style = detail::stringOr(json, QStringLiteral("style"));
    request.d->background = detail::stringOr(json, QStringLiteral("background"));
    request.d->outputFormat = detail::stringOr(json, QStringLiteral("output_format"));
    request.d->moderation = detail::stringOr(json, QStringLiteral("moderation"));
    request.d->user = detail::stringOr(json, QStringLiteral("user"));
    return request;
}

bool ImageGenerationRequest::operator==(const ImageGenerationRequest &other) const
{
    return d->prompt == other.d->prompt && d->model == other.d->model && d->n == other.d->n
           && d->size == other.d->size && d->quality == other.d->quality
           && d->responseFormat == other.d->responseFormat && d->style == other.d->style
           && d->background == other.d->background && d->outputFormat == other.d->outputFormat
           && d->moderation == other.d->moderation && d->user == other.d->user
           && d->extraBody == other.d->extraBody;
}

} // namespace Core
} // namespace QtOpenAi
