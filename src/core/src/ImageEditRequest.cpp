// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ImageEditRequest.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ImageEditRequestData : public QSharedData
{
public:
    QList<ImageEditRequest::ImageFile> images;
    QString maskFileName;
    QByteArray maskData;
    bool hasMask = false;
    QString prompt;
    QString model;
    std::optional<int> n;
    QString size;
    QString responseFormat;
    QString background;
    QString outputFormat;
    QString user;
};

ImageEditRequest::ImageEditRequest()
    : d(new ImageEditRequestData)
{ }

ImageEditRequest::ImageEditRequest(QByteArray imageData, QString fileName, QString prompt,
                                   QString model)
    : d(new ImageEditRequestData)
{
    d->images.append({std::move(fileName), std::move(imageData)});
    d->prompt = std::move(prompt);
    d->model = std::move(model);
}

ImageEditRequest::ImageEditRequest(const ImageEditRequest &other) = default;
ImageEditRequest::ImageEditRequest(ImageEditRequest &&other) noexcept = default;
ImageEditRequest &ImageEditRequest::operator=(const ImageEditRequest &other) = default;
ImageEditRequest &ImageEditRequest::operator=(ImageEditRequest &&other) noexcept = default;
ImageEditRequest::~ImageEditRequest() = default;

QList<ImageEditRequest::ImageFile> ImageEditRequest::images() const { return d->images; }
void ImageEditRequest::setImages(const QList<ImageFile> &images) { d->images = images; }
void ImageEditRequest::addImage(const QString &fileName, const QByteArray &data)
{
    d->images.append({fileName, data});
}

QByteArray ImageEditRequest::maskData() const { return d->maskData; }
QString ImageEditRequest::maskFileName() const { return d->maskFileName; }
void ImageEditRequest::setMask(const QString &fileName, const QByteArray &data)
{
    d->maskFileName = fileName;
    d->maskData = data;
    d->hasMask = true;
}
bool ImageEditRequest::hasMask() const { return d->hasMask; }

QString ImageEditRequest::prompt() const { return d->prompt; }
void ImageEditRequest::setPrompt(const QString &prompt) { d->prompt = prompt; }

QString ImageEditRequest::model() const { return d->model; }
void ImageEditRequest::setModel(const QString &model) { d->model = model; }

std::optional<int> ImageEditRequest::n() const { return d->n; }
void ImageEditRequest::setN(int n) { d->n = n; }

QString ImageEditRequest::size() const { return d->size; }
void ImageEditRequest::setSize(const QString &size) { d->size = size; }

QString ImageEditRequest::responseFormat() const { return d->responseFormat; }
void ImageEditRequest::setResponseFormat(const QString &format) { d->responseFormat = format; }

QString ImageEditRequest::background() const { return d->background; }
void ImageEditRequest::setBackground(const QString &background) { d->background = background; }

QString ImageEditRequest::outputFormat() const { return d->outputFormat; }
void ImageEditRequest::setOutputFormat(const QString &outputFormat)
{
    d->outputFormat = outputFormat;
}

QString ImageEditRequest::user() const { return d->user; }
void ImageEditRequest::setUser(const QString &user) { d->user = user; }

QList<ImageEditRequest::FormField> ImageEditRequest::formFields() const
{
    QList<FormField> fields;
    fields.append({QStringLiteral("prompt"), d->prompt});
    if (!d->model.isEmpty())
        fields.append({QStringLiteral("model"), d->model});
    if (d->n)
        fields.append({QStringLiteral("n"), QString::number(*d->n)});
    if (!d->size.isEmpty())
        fields.append({QStringLiteral("size"), d->size});
    if (!d->responseFormat.isEmpty())
        fields.append({QStringLiteral("response_format"), d->responseFormat});
    if (!d->background.isEmpty())
        fields.append({QStringLiteral("background"), d->background});
    if (!d->outputFormat.isEmpty())
        fields.append({QStringLiteral("output_format"), d->outputFormat});
    if (!d->user.isEmpty())
        fields.append({QStringLiteral("user"), d->user});
    return fields;
}

bool ImageEditRequest::operator==(const ImageEditRequest &other) const
{
    return d->images == other.d->images && d->maskFileName == other.d->maskFileName
           && d->maskData == other.d->maskData && d->hasMask == other.d->hasMask
           && d->prompt == other.d->prompt && d->model == other.d->model && d->n == other.d->n
           && d->size == other.d->size && d->responseFormat == other.d->responseFormat
           && d->background == other.d->background && d->outputFormat == other.d->outputFormat
           && d->user == other.d->user;
}

} // namespace Core
} // namespace QtOpenAi
