// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/FineTuningJobPoller.h"

#include "QtOpenAi/Client/Client.h"
#include "QtOpenAi/Client/FineTuningJobReply.h"

#include "JobPoller_p.h"

namespace QtOpenAi {
namespace Client {

class FineTuningJobPollerPrivate : public JobPollerPrivate
{
public:
    Core::FineTuningJob job;
};

FineTuningJobPoller::FineTuningJobPoller(Client *client, QString jobId, int intervalMs,
                                         QObject *parent)
    : JobPoller(*new FineTuningJobPollerPrivate, client, std::move(jobId), intervalMs, parent)
{ }

Core::FineTuningJob FineTuningJobPoller::job() const
{
    Q_D(const FineTuningJobPoller);
    return d->job;
}

void FineTuningJobPoller::requestPoll()
{
    trackPoll<FineTuningJobReply, Core::FineTuningJob>(client()->getFineTuningJob(jobId()),
                                                       [this](const Core::FineTuningJob &job) {
                                                           Q_D(FineTuningJobPoller);
                                                           d->job = job;
                                                           Q_EMIT progressed(job);
                                                           if (!job.isTerminal())
                                                               return false;
                                                           finish();
                                                           Q_EMIT completed(job);
                                                           return true;
                                                       });
}

} // namespace Client
} // namespace QtOpenAi
