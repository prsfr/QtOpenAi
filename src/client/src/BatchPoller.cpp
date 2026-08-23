// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/BatchPoller.h"

#include "QtOpenAi/Client/BatchReply.h"
#include "QtOpenAi/Client/Client.h"

#include "JobPoller_p.h"

namespace QtOpenAi {
namespace Client {

class BatchPollerPrivate : public JobPollerPrivate
{
public:
    Core::Batch batch;
};

BatchPoller::BatchPoller(Client *client, QString batchId, int intervalMs, QObject *parent)
    : JobPoller(*new BatchPollerPrivate, client, std::move(batchId), intervalMs, parent)
{ }

Core::Batch BatchPoller::batch() const
{
    Q_D(const BatchPoller);
    return d->batch;
}

void BatchPoller::requestPoll()
{
    Q_D(BatchPoller);
    trackTerminalPoll<BatchReply, Core::Batch>(client()->getBatch(jobId()), d,
                                               &BatchPollerPrivate::batch, &BatchPoller::progressed,
                                               &BatchPoller::completed);
}

} // namespace Client
} // namespace QtOpenAi
