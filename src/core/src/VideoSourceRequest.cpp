// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/VideoSourceRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class VideoSourceRequestData : public QSharedData
{
public:
    QString prompt;
    QString sourceVideoId;
    QByteArray sourceVideoData;
    QString sourceVideoFileName;
    QString seconds;
    QJsonObject extraBody;
};

VideoSourceRequest::VideoSourceRequest()
    : d(new VideoSourceRequestData)
{ }

VideoSourceRequest::VideoSourceRequest(QString sourceVideoId, QString prompt)
    : d(new VideoSourceRequestData)
{
    d->sourceVideoId = std::move(sourceVideoId);
    d->prompt = std::move(prompt);
}

VideoSourceRequest::VideoSourceRequest(const VideoSourceRequest &other) = default;
VideoSourceRequest::VideoSourceRequest(VideoSourceRequest &&other) noexcept = default;
VideoSourceRequest &VideoSourceRequest::operator=(const VideoSourceRequest &other) = default;
VideoSourceRequest &VideoSourceRequest::operator=(VideoSourceRequest &&other) noexcept = default;
VideoSourceRequest::~VideoSourceRequest() = default;

QString VideoSourceRequest::prompt() const { return d->prompt; }
void VideoSourceRequest::setPrompt(const QString &prompt) { d->prompt = prompt; }

QString VideoSourceRequest::sourceVideoId() const { return d->sourceVideoId; }

void VideoSourceRequest::setSourceVideoId(const QString &videoId)
{
    d->sourceVideoId = videoId;
    // Exclusive with an upload: see the class comment. Clearing here rather
    // than refusing later means the last call always wins, visibly.
    d->sourceVideoData.clear();
    d->sourceVideoFileName.clear();
}

QByteArray VideoSourceRequest::sourceVideoData() const { return d->sourceVideoData; }
QString VideoSourceRequest::sourceVideoFileName() const { return d->sourceVideoFileName; }

void VideoSourceRequest::setSourceVideo(const QString &fileName, const QByteArray &data)
{
    d->sourceVideoFileName = fileName;
    d->sourceVideoData = data;
    d->sourceVideoId.clear();
}

bool VideoSourceRequest::hasSourceUpload() const { return !d->sourceVideoData.isEmpty(); }

QString VideoSourceRequest::seconds() const { return d->seconds; }
void VideoSourceRequest::setSeconds(const QString &seconds) { d->seconds = seconds; }

QJsonObject VideoSourceRequest::extraBody() const { return d->extraBody; }
void VideoSourceRequest::setExtraBody(const QJsonObject &extra) { d->extraBody = extra; }

QList<VideoSourceRequest::FormField> VideoSourceRequest::formFields(bool withSeconds) const
{
    QList<FormField> fields;
    if (!d->prompt.isEmpty())
        fields.append({QStringLiteral("prompt"), d->prompt});
    if (withSeconds && !d->seconds.isEmpty())
        fields.append({QStringLiteral("seconds"), d->seconds});
    // The video itself is the file part and is added by the Client; a source
    // named by id has no place in a multipart body, since naming one is
    // precisely the case that does not need an upload.
    return fields;
}

QJsonObject VideoSourceRequest::toJson(bool withSeconds) const
{
    QJsonObject json;
    // `video` is an object with an `id`, not a bare string -- the JSON variant
    // of these endpoints takes a VideoReferenceInputParam. Flattening it to a
    // string is the mistake this shape exists to prevent.
    if (!d->sourceVideoId.isEmpty()) {
        QJsonObject reference;
        reference.insert(QStringLiteral("id"), d->sourceVideoId);
        json.insert(QStringLiteral("video"), reference);
    }
    detail::insertIfNotEmpty(json, QStringLiteral("prompt"), d->prompt);
    if (withSeconds)
        detail::insertIfNotEmpty(json, QStringLiteral("seconds"), d->seconds);
    for (auto it = d->extraBody.constBegin(); it != d->extraBody.constEnd(); ++it)
        json.insert(it.key(), it.value());
    return json;
}

bool VideoSourceRequest::operator==(const VideoSourceRequest &other) const
{
    return d->prompt == other.d->prompt && d->sourceVideoId == other.d->sourceVideoId
           && d->sourceVideoData == other.d->sourceVideoData
           && d->sourceVideoFileName == other.d->sourceVideoFileName
           && d->seconds == other.d->seconds && d->extraBody == other.d->extraBody;
}

} // namespace Core
} // namespace QtOpenAi
