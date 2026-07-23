// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ImageResponse.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

// --- Image -----------------------------------------------------------------

class ImageData : public QSharedData
{
public:
    QString url;
    QString b64Json;
    QString revisedPrompt;
};

Image::Image()
    : d(new ImageData)
{ }

Image::Image(const Image &other) = default;
Image::Image(Image &&other) noexcept = default;
Image &Image::operator=(const Image &other) = default;
Image &Image::operator=(Image &&other) noexcept = default;
Image::~Image() = default;

QString Image::url() const { return d->url; }
void Image::setUrl(const QString &url) { d->url = url; }

QString Image::b64Json() const { return d->b64Json; }
void Image::setB64Json(const QString &b64Json) { d->b64Json = b64Json; }

QString Image::revisedPrompt() const { return d->revisedPrompt; }
void Image::setRevisedPrompt(const QString &revisedPrompt) { d->revisedPrompt = revisedPrompt; }

QJsonObject Image::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("url"), d->url);
    detail::insertIfNotEmpty(json, QStringLiteral("b64_json"), d->b64Json);
    detail::insertIfNotEmpty(json, QStringLiteral("revised_prompt"), d->revisedPrompt);
    return json;
}

Image Image::fromJson(const QJsonObject &json)
{
    Image image;
    image.d->url = detail::stringOr(json, QStringLiteral("url"));
    image.d->b64Json = detail::stringOr(json, QStringLiteral("b64_json"));
    image.d->revisedPrompt = detail::stringOr(json, QStringLiteral("revised_prompt"));
    return image;
}

bool Image::operator==(const Image &other) const
{
    return d->url == other.d->url && d->b64Json == other.d->b64Json
           && d->revisedPrompt == other.d->revisedPrompt;
}

// --- ImageResponse ---------------------------------------------------------

class ImageResponseData : public QSharedData
{
public:
    qint64 created = 0;
    QList<Image> data;
    std::optional<Usage> usage;
};

ImageResponse::ImageResponse()
    : d(new ImageResponseData)
{ }

ImageResponse::ImageResponse(const ImageResponse &other) = default;
ImageResponse::ImageResponse(ImageResponse &&other) noexcept = default;
ImageResponse &ImageResponse::operator=(const ImageResponse &other) = default;
ImageResponse &ImageResponse::operator=(ImageResponse &&other) noexcept = default;
ImageResponse::~ImageResponse() = default;

qint64 ImageResponse::created() const { return d->created; }
void ImageResponse::setCreated(qint64 created) { d->created = created; }

QList<Image> ImageResponse::data() const { return d->data; }
void ImageResponse::setData(const QList<Image> &data) { d->data = data; }

std::optional<Usage> ImageResponse::usage() const { return d->usage; }
void ImageResponse::setUsage(const Usage &usage) { d->usage = usage; }

Image ImageResponse::firstImage() const { return d->data.isEmpty() ? Image() : d->data.first(); }

QJsonObject ImageResponse::toJson() const
{
    QJsonObject json;
    if (d->created != 0)
        json.insert(QStringLiteral("created"), d->created);
    QJsonArray data;
    for (const Image &image : d->data)
        data.append(image.toJson());
    json.insert(QStringLiteral("data"), data);
    if (d->usage)
        json.insert(QStringLiteral("usage"), d->usage->toJson());
    return json;
}

ImageResponse ImageResponse::fromJson(const QJsonObject &json)
{
    ImageResponse response;
    response.d->created = static_cast<qint64>(json.value(QStringLiteral("created")).toDouble());
    const QJsonArray data = json.value(QStringLiteral("data")).toArray();
    for (const QJsonValue &value : data)
        response.d->data.append(Image::fromJson(value.toObject()));
    if (json.contains(QStringLiteral("usage")))
        response.d->usage = Usage::fromJson(json.value(QStringLiteral("usage")).toObject());
    return response;
}

bool ImageResponse::operator==(const ImageResponse &other) const
{
    return d->created == other.d->created && d->data == other.d->data && d->usage == other.d->usage;
}

} // namespace Core
} // namespace QtOpenAi
