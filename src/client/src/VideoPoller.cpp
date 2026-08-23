// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VideoPoller.h"

#include "QtOpenAi/Client/Client.h"
#include "QtOpenAi/Client/VideoReply.h"

#include "JobPoller_p.h"

namespace QtOpenAi {
namespace Client {

class VideoPollerPrivate : public JobPollerPrivate
{
public:
    Core::VideoJob job;
};

VideoPoller::VideoPoller(Client *client, QString videoId, int intervalMs, QObject *parent)
    : JobPoller(*new VideoPollerPrivate, client, std::move(videoId), intervalMs, parent)
{ }

Core::VideoJob VideoPoller::job() const
{
    Q_D(const VideoPoller);
    return d->job;
}

void VideoPoller::requestPoll()
{
    Q_D(VideoPoller);
    trackTerminalPoll<VideoReply, Core::VideoJob>(
            client()->getVideo(jobId()), d, &VideoPollerPrivate::job, &VideoPoller::progressed,
            &VideoPoller::completed);
}

} // namespace Client
} // namespace QtOpenAi
