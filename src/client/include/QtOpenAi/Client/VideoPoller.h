// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Core/VideoJob.h>

#include <QtCore/QObject>

namespace QtOpenAi {
namespace Client {

class Client;
class VideoPollerPrivate;

// A signal-based poll-until-complete helper for Sora video jobs.
//
// Because rendering is asynchronous, VideoPoller repeatedly issues GET
// /videos/{id} on a timer (via the owning Client) and reports every observed
// state through progressed(). It stops automatically once the job reaches a
// terminal state — emitting completed() — or when a request fails, emitting
// failed(). Created by Client::pollVideo(); auto-deletes after it stops unless
// disabled.
class QTOPENAI_CLIENT_EXPORT VideoPoller : public QObject
{
    Q_OBJECT
public:
    ~VideoPoller() override;

    QString videoId() const;

    // Delay between successive polls in milliseconds (default 2000).
    int pollIntervalMs() const;
    void setPollIntervalMs(int intervalMs);

    bool isPolling() const;
    bool isFinished() const;

    // The most recently observed job state.
    Core::VideoJob job() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    // Begin polling (issues the first GET immediately). No-op if already polling.
    void start();
    // Stop polling without emitting completed()/failed().
    void stop();

Q_SIGNALS:
    // Emitted after every successful poll with the current job state, including
    // the terminal one.
    void progressed(const QtOpenAi::Core::VideoJob &job);
    // Emitted once when the job reaches a terminal state (Completed or Failed).
    void completed(const QtOpenAi::Core::VideoJob &job);
    // Emitted once when a poll request itself fails (network/HTTP/parse).
    void failed(const QtOpenAi::Client::ClientError &error);

private:
    friend class Client;
    VideoPoller(Client *client, QString videoId, int intervalMs, QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(VideoPoller)
    QScopedPointer<VideoPollerPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
