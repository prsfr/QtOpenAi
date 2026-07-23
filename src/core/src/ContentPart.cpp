// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ContentPart.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ContentPartData : public QSharedData
{
public:
    QString type;
    QString text;
    QString imageUrl;
    QString imageDetail;
    QString audioData;
    QString audioFormat;
    QString fileId;
    QString fileData;
    QString fileName;
};

ContentPart::ContentPart()
    : d(new ContentPartData)
{ }

ContentPart::ContentPart(const QString &type)
    : d(new ContentPartData)
{
    d->type = type;
}

ContentPart::ContentPart(const ContentPart &other) = default;
ContentPart::ContentPart(ContentPart &&other) noexcept = default;
ContentPart &ContentPart::operator=(const ContentPart &other) = default;
ContentPart &ContentPart::operator=(ContentPart &&other) noexcept = default;
ContentPart::~ContentPart() = default;

QString ContentPart::type() const { return d->type; }
void ContentPart::setType(const QString &type) { d->type = type; }

QString ContentPart::text() const { return d->text; }
void ContentPart::setText(const QString &text) { d->text = text; }

QString ContentPart::imageUrl() const { return d->imageUrl; }
void ContentPart::setImageUrl(const QString &imageUrl) { d->imageUrl = imageUrl; }

QString ContentPart::imageDetail() const { return d->imageDetail; }
void ContentPart::setImageDetail(const QString &detail) { d->imageDetail = detail; }

QString ContentPart::audioData() const { return d->audioData; }
void ContentPart::setAudioData(const QString &audioData) { d->audioData = audioData; }

QString ContentPart::audioFormat() const { return d->audioFormat; }
void ContentPart::setAudioFormat(const QString &format) { d->audioFormat = format; }

QString ContentPart::fileId() const { return d->fileId; }
void ContentPart::setFileId(const QString &fileId) { d->fileId = fileId; }

QString ContentPart::fileData() const { return d->fileData; }
void ContentPart::setFileData(const QString &fileData) { d->fileData = fileData; }

QString ContentPart::fileName() const { return d->fileName; }
void ContentPart::setFileName(const QString &fileName) { d->fileName = fileName; }

ContentPart ContentPart::text(const QString &text)
{
    ContentPart part(QStringLiteral("text"));
    part.d->text = text;
    return part;
}

ContentPart ContentPart::imageUrl(const QString &url, const QString &detail)
{
    ContentPart part(QStringLiteral("image_url"));
    part.d->imageUrl = url;
    part.d->imageDetail = detail;
    return part;
}

ContentPart ContentPart::inputAudio(const QString &base64Data, const QString &format)
{
    ContentPart part(QStringLiteral("input_audio"));
    part.d->audioData = base64Data;
    part.d->audioFormat = format;
    return part;
}

ContentPart ContentPart::file(const QString &fileId)
{
    ContentPart part(QStringLiteral("file"));
    part.d->fileId = fileId;
    return part;
}

QJsonObject ContentPart::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("type"), d->type);
    if (d->type == QLatin1String("text")) {
        json.insert(QStringLiteral("text"), d->text);
    } else if (d->type == QLatin1String("image_url")) {
        QJsonObject image;
        image.insert(QStringLiteral("url"), d->imageUrl);
        detail::insertIfNotEmpty(image, QStringLiteral("detail"), d->imageDetail);
        json.insert(QStringLiteral("image_url"), image);
    } else if (d->type == QLatin1String("input_audio")) {
        QJsonObject audio;
        audio.insert(QStringLiteral("data"), d->audioData);
        audio.insert(QStringLiteral("format"), d->audioFormat);
        json.insert(QStringLiteral("input_audio"), audio);
    } else if (d->type == QLatin1String("file")) {
        QJsonObject file;
        detail::insertIfNotEmpty(file, QStringLiteral("file_id"), d->fileId);
        detail::insertIfNotEmpty(file, QStringLiteral("file_data"), d->fileData);
        detail::insertIfNotEmpty(file, QStringLiteral("filename"), d->fileName);
        json.insert(QStringLiteral("file"), file);
    }
    return json;
}

ContentPart ContentPart::fromJson(const QJsonObject &json)
{
    ContentPart part;
    part.d->type = detail::stringOr(json, QStringLiteral("type"));
    part.d->text = detail::stringOr(json, QStringLiteral("text"));

    const QJsonObject image = json.value(QStringLiteral("image_url")).toObject();
    part.d->imageUrl = detail::stringOr(image, QStringLiteral("url"));
    part.d->imageDetail = detail::stringOr(image, QStringLiteral("detail"));

    const QJsonObject audio = json.value(QStringLiteral("input_audio")).toObject();
    part.d->audioData = detail::stringOr(audio, QStringLiteral("data"));
    part.d->audioFormat = detail::stringOr(audio, QStringLiteral("format"));

    const QJsonObject file = json.value(QStringLiteral("file")).toObject();
    part.d->fileId = detail::stringOr(file, QStringLiteral("file_id"));
    part.d->fileData = detail::stringOr(file, QStringLiteral("file_data"));
    part.d->fileName = detail::stringOr(file, QStringLiteral("filename"));
    return part;
}

bool ContentPart::operator==(const ContentPart &other) const
{
    return d->type == other.d->type && d->text == other.d->text && d->imageUrl == other.d->imageUrl
           && d->imageDetail == other.d->imageDetail && d->audioData == other.d->audioData
           && d->audioFormat == other.d->audioFormat && d->fileId == other.d->fileId
           && d->fileData == other.d->fileData && d->fileName == other.d->fileName;
}

} // namespace Core
} // namespace QtOpenAi
