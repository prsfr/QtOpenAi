// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/Enums.h>
#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class VideoJobData;

// One asynchronous video-generation job (POST /videos, GET /videos/{id}, ...).
//
// Sora renders video off-line: creating a job returns it in the `queued` state
// and the client polls GET /videos/{id} until `progress` reaches 100 and the
// status becomes `completed` (or `failed`). The rendered bytes are then fetched
// separately from GET /videos/{id}/content.
class QTOPENAI_CORE_EXPORT VideoJob
{
public:
    VideoJob();
    VideoJob(const VideoJob &other);
    VideoJob(VideoJob &&other) noexcept;
    VideoJob &operator=(const VideoJob &other);
    VideoJob &operator=(VideoJob &&other) noexcept;
    ~VideoJob();

    void swap(VideoJob &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    VideoStatus status() const;
    void setStatus(VideoStatus status);

    // Render progress as a percentage in [0, 100].
    int progress() const;
    void setProgress(int progress);

    QString model() const;
    void setModel(const QString &model);

    // Frame size, e.g. "720x1280"; empty when absent.
    QString size() const;
    void setSize(const QString &size);

    // Clip duration in seconds as a string (e.g. "8"), matching the wire type.
    QString seconds() const;
    void setSeconds(const QString &seconds);

    // Unix creation timestamp (`created_at`); 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    // Unix completion timestamp (`completed_at`); 0 while still rendering.
    qint64 completedAt() const;
    void setCompletedAt(qint64 completedAt);

    // The failure code/message from the `error` object (populated when the job
    // status is Failed); both empty otherwise.
    QString errorCode() const;
    void setErrorCode(const QString &errorCode);

    QString errorMessage() const;
    void setErrorMessage(const QString &errorMessage);

    // True once the job has reached a terminal state (Completed or Failed) and
    // will no longer change; polling can stop.
    bool isTerminal() const;

    QJsonObject toJson() const;
    static VideoJob fromJson(const QJsonObject &json);

    bool operator==(const VideoJob &other) const;
    bool operator!=(const VideoJob &other) const { return !(*this == other); }

private:
    QSharedDataPointer<VideoJobData> d;
};

// A `list` of video jobs (GET /videos). Cursor-paginated; reuses the shared
// list-page type.
using VideoList = ListPage<VideoJob>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::VideoJob)
Q_DECLARE_METATYPE(QtOpenAi::Core::VideoJob)
Q_DECLARE_METATYPE(QtOpenAi::Core::VideoList)
