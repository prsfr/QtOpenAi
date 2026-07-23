// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ImageVariationRequest.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ImageVariationRequestData : public QSharedData
{
public:
    QByteArray imageData;
    QString fileName;
    QString model;
    std::optional<int> n;
    QString size;
    QString responseFormat;
    QString user;
};

ImageVariationRequest::ImageVariationRequest()
    : d(new ImageVariationRequestData)
{ }

ImageVariationRequest::ImageVariationRequest(QByteArray imageData, QString fileName, QString model)
    : d(new ImageVariationRequestData)
{
    d->imageData = std::move(imageData);
    d->fileName = std::move(fileName);
    d->model = std::move(model);
}

ImageVariationRequest::ImageVariationRequest(const ImageVariationRequest &other) = default;
ImageVariationRequest::ImageVariationRequest(ImageVariationRequest &&other) noexcept = default;
ImageVariationRequest &ImageVariationRequest::operator=(const ImageVariationRequest &other)
        = default;
ImageVariationRequest &ImageVariationRequest::operator=(ImageVariationRequest &&other) noexcept
        = default;
ImageVariationRequest::~ImageVariationRequest() = default;

QByteArray ImageVariationRequest::imageData() const { return d->imageData; }
void ImageVariationRequest::setImageData(const QByteArray &imageData) { d->imageData = imageData; }

QString ImageVariationRequest::fileName() const { return d->fileName; }
void ImageVariationRequest::setFileName(const QString &fileName) { d->fileName = fileName; }

QString ImageVariationRequest::model() const { return d->model; }
void ImageVariationRequest::setModel(const QString &model) { d->model = model; }

std::optional<int> ImageVariationRequest::n() const { return d->n; }
void ImageVariationRequest::setN(int n) { d->n = n; }

QString ImageVariationRequest::size() const { return d->size; }
void ImageVariationRequest::setSize(const QString &size) { d->size = size; }

QString ImageVariationRequest::responseFormat() const { return d->responseFormat; }
void ImageVariationRequest::setResponseFormat(const QString &format) { d->responseFormat = format; }

QString ImageVariationRequest::user() const { return d->user; }
void ImageVariationRequest::setUser(const QString &user) { d->user = user; }

QList<ImageVariationRequest::FormField> ImageVariationRequest::formFields() const
{
    QList<FormField> fields;
    if (!d->model.isEmpty())
        fields.append({QStringLiteral("model"), d->model});
    if (d->n)
        fields.append({QStringLiteral("n"), QString::number(*d->n)});
    if (!d->size.isEmpty())
        fields.append({QStringLiteral("size"), d->size});
    if (!d->responseFormat.isEmpty())
        fields.append({QStringLiteral("response_format"), d->responseFormat});
    if (!d->user.isEmpty())
        fields.append({QStringLiteral("user"), d->user});
    return fields;
}

bool ImageVariationRequest::operator==(const ImageVariationRequest &other) const
{
    return d->imageData == other.d->imageData && d->fileName == other.d->fileName
           && d->model == other.d->model && d->n == other.d->n && d->size == other.d->size
           && d->responseFormat == other.d->responseFormat && d->user == other.d->user;
}

} // namespace Core
} // namespace QtOpenAi
