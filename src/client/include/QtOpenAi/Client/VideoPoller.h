// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/JobPoller.h>
#include <QtOpenAi/Core/VideoJob.h>

namespace QtOpenAi {
namespace Client {

class VideoPollerPrivate;

// A signal-based poll-until-complete helper for Sora video jobs.
//
// Because rendering is asynchronous, VideoPoller repeatedly issues GET
// /videos/{id} on a timer (via the owning Client) and reports every observed
// state through progressed(). It stops automatically once the job reaches a
// terminal state — emitting completed() — or when a request fails, emitting
// JobPoller::failed(). Created by Client::pollVideo(); auto-deletes after it
// stops unless disabled.
//
// The timer, the lifecycle flags and the auto-delete policy live in JobPoller,
// which BatchPoller shares.
class QTOPENAI_CLIENT_EXPORT VideoPoller : public JobPoller
{
    Q_OBJECT
public:
    // The polled video's id — jobId() spelled the way this endpoint spells it.
    QString videoId() const { return jobId(); }

    // The most recently observed job state.
    Core::VideoJob job() const;

Q_SIGNALS:
    // Emitted after every successful poll with the current job state, including
    // the terminal one.
    void progressed(const QtOpenAi::Core::VideoJob &job);
    // Emitted once when the job reaches a terminal state (Completed or Failed).
    void completed(const QtOpenAi::Core::VideoJob &job);

private:
    friend class Client;
    VideoPoller(Client *client, QString videoId, int intervalMs, QObject *parent = nullptr);

    void requestPoll() override;

    Q_DECLARE_PRIVATE(VideoPoller)
};

} // namespace Client
} // namespace QtOpenAi
