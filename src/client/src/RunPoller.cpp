// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/RunPoller.h"

#include "QtOpenAi/Client/Client.h"
#include "QtOpenAi/Client/RunReply.h"

#include "JobPoller_p.h"

namespace QtOpenAi {
namespace Client {

class RunPollerPrivate : public JobPollerPrivate
{
public:
    QString threadId;
    Core::Run run;
};

RunPoller::RunPoller(Client *client, QString threadId, QString runId, int intervalMs,
                     QObject *parent)
    : JobPoller(*new RunPollerPrivate, client, std::move(runId), intervalMs, parent)
{
    Q_D(RunPoller);
    d->threadId = std::move(threadId);
}

QString RunPoller::threadId() const
{
    Q_D(const RunPoller);
    return d->threadId;
}

Core::Run RunPoller::run() const
{
    Q_D(const RunPoller);
    return d->run;
}

void RunPoller::requestPoll()
{
    Q_D(RunPoller);
    trackPoll<RunReply, Core::Run>(client()->getRun(d->threadId, jobId()),
                                   [this](const Core::Run &run) {
                                       Q_D(RunPoller);
                                       d->run = run;
                                       Q_EMIT progressed(run);
                                       // A parked run never moves on its own, so
                                       // stop for it as well as for a terminal
                                       // state -- with its own signal, because
                                       // the caller has work to do.
                                       if (run.requiresAction()) {
                                           finish();
                                           Q_EMIT requiresAction(run);
                                           return true;
                                       }
                                       if (!run.isTerminal())
                                           return false;
                                       finish();
                                       Q_EMIT completed(run);
                                       return true;
                                   });
}

} // namespace Client
} // namespace QtOpenAi
