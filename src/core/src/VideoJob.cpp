// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/VideoJob.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class VideoJobData : public QSharedData
{
public:
    QString id;
    VideoStatus status = VideoStatus::Queued;
    int progress = 0;
    QString model;
    QString size;
    QString seconds;
    QString prompt;
    QString remixedFromVideoId;
    qint64 createdAt = 0;
    qint64 completedAt = 0;
    qint64 expiresAt = 0;
    QString errorCode;
    QString errorMessage;
};

VideoJob::VideoJob()
    : d(new VideoJobData)
{ }

VideoJob::VideoJob(const VideoJob &other) = default;
VideoJob::VideoJob(VideoJob &&other) noexcept = default;
VideoJob &VideoJob::operator=(const VideoJob &other) = default;
VideoJob &VideoJob::operator=(VideoJob &&other) noexcept = default;
VideoJob::~VideoJob() = default;

QString VideoJob::id() const { return d->id; }
void VideoJob::setId(const QString &id) { d->id = id; }

VideoStatus VideoJob::status() const { return d->status; }
void VideoJob::setStatus(VideoStatus status) { d->status = status; }

int VideoJob::progress() const { return d->progress; }
void VideoJob::setProgress(int progress) { d->progress = progress; }

QString VideoJob::model() const { return d->model; }
void VideoJob::setModel(const QString &model) { d->model = model; }

QString VideoJob::size() const { return d->size; }
void VideoJob::setSize(const QString &size) { d->size = size; }

QString VideoJob::seconds() const { return d->seconds; }
void VideoJob::setSeconds(const QString &seconds) { d->seconds = seconds; }

qint64 VideoJob::createdAt() const { return d->createdAt; }
void VideoJob::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

qint64 VideoJob::completedAt() const { return d->completedAt; }
void VideoJob::setCompletedAt(qint64 completedAt) { d->completedAt = completedAt; }

qint64 VideoJob::expiresAt() const { return d->expiresAt; }
void VideoJob::setExpiresAt(qint64 expiresAt) { d->expiresAt = expiresAt; }

QString VideoJob::prompt() const { return d->prompt; }
void VideoJob::setPrompt(const QString &prompt) { d->prompt = prompt; }

QString VideoJob::remixedFromVideoId() const { return d->remixedFromVideoId; }
void VideoJob::setRemixedFromVideoId(const QString &videoId) { d->remixedFromVideoId = videoId; }

QString VideoJob::errorCode() const { return d->errorCode; }
void VideoJob::setErrorCode(const QString &errorCode) { d->errorCode = errorCode; }

QString VideoJob::errorMessage() const { return d->errorMessage; }
void VideoJob::setErrorMessage(const QString &errorMessage) { d->errorMessage = errorMessage; }

bool VideoJob::isTerminal() const { return Core::isTerminal(d->status); }

QJsonObject VideoJob::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("object"), QStringLiteral("video"));
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    json.insert(QStringLiteral("status"), videoStatusToString(d->status));
    json.insert(QStringLiteral("progress"), d->progress);
    detail::insertIfNotEmpty(json, QStringLiteral("model"), d->model);
    detail::insertIfNotEmpty(json, QStringLiteral("size"), d->size);
    detail::insertIfNotEmpty(json, QStringLiteral("seconds"), d->seconds);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNonZero(json, QStringLiteral("completed_at"), d->completedAt);
    detail::insertIfNonZero(json, QStringLiteral("expires_at"), d->expiresAt);
    detail::insertIfNotEmpty(json, QStringLiteral("prompt"), d->prompt);
    detail::insertIfNotEmpty(json, QStringLiteral("remixed_from_video_id"), d->remixedFromVideoId);
    if (!d->errorCode.isEmpty() || !d->errorMessage.isEmpty()) {
        QJsonObject error;
        detail::insertIfNotEmpty(error, QStringLiteral("code"), d->errorCode);
        detail::insertIfNotEmpty(error, QStringLiteral("message"), d->errorMessage);
        json.insert(QStringLiteral("error"), error);
    }
    return json;
}

VideoJob VideoJob::fromJson(const QJsonObject &json)
{
    VideoJob job;
    job.d->id = detail::stringOr(json, QStringLiteral("id"));
    job.d->status = videoStatusFromString(detail::stringOr(json, QStringLiteral("status")));
    job.d->progress = json.value(QStringLiteral("progress")).toInt();
    job.d->model = detail::stringOr(json, QStringLiteral("model"));
    job.d->size = detail::stringOr(json, QStringLiteral("size"));
    job.d->seconds = detail::stringOr(json, QStringLiteral("seconds"));
    job.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    job.d->completedAt = detail::int64Or(json, QStringLiteral("completed_at"));
    job.d->expiresAt = detail::int64Or(json, QStringLiteral("expires_at"));
    job.d->prompt = detail::stringOr(json, QStringLiteral("prompt"));
    job.d->remixedFromVideoId = detail::stringOr(json, QStringLiteral("remixed_from_video_id"));
    const QJsonValue error = json.value(QStringLiteral("error"));
    if (error.isObject()) {
        const QJsonObject errorObject = error.toObject();
        job.d->errorCode = detail::stringOr(errorObject, QStringLiteral("code"));
        job.d->errorMessage = detail::stringOr(errorObject, QStringLiteral("message"));
    }
    return job;
}

bool VideoJob::operator==(const VideoJob &other) const
{
    return d->id == other.d->id && d->status == other.d->status && d->progress == other.d->progress
           && d->model == other.d->model && d->size == other.d->size
           && d->seconds == other.d->seconds && d->createdAt == other.d->createdAt
           && d->completedAt == other.d->completedAt && d->expiresAt == other.d->expiresAt
           && d->prompt == other.d->prompt && d->remixedFromVideoId == other.d->remixedFromVideoId
           && d->errorCode == other.d->errorCode && d->errorMessage == other.d->errorMessage;
}

} // namespace Core
} // namespace QtOpenAi
