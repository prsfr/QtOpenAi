// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/EvalRunPoller.h"

#include "QtOpenAi/Client/Client.h"
#include "QtOpenAi/Client/EvalRunReply.h"

#include "JobPoller_p.h"

namespace QtOpenAi {
namespace Client {

class EvalRunPollerPrivate : public JobPollerPrivate
{
public:
    QString evalId;
    Core::EvalRun run;
};

EvalRunPoller::EvalRunPoller(Client *client, QString evalId, QString runId, int intervalMs,
                             QObject *parent)
    : JobPoller(*new EvalRunPollerPrivate, client, std::move(runId), intervalMs, parent)
{
    Q_D(EvalRunPoller);
    d->evalId = std::move(evalId);
}

QString EvalRunPoller::evalId() const
{
    Q_D(const EvalRunPoller);
    return d->evalId;
}

Core::EvalRun EvalRunPoller::run() const
{
    Q_D(const EvalRunPoller);
    return d->run;
}

void EvalRunPoller::requestPoll()
{
    Q_D(EvalRunPoller);
    trackPoll<EvalRunReply, Core::EvalRun>(client()->getEvalRun(d->evalId, jobId()),
                                           [this](const Core::EvalRun &run) {
                                               Q_D(EvalRunPoller);
                                               d->run = run;
                                               Q_EMIT progressed(run);
                                               if (!run.isTerminal())
                                                   return false;
                                               finish();
                                               Q_EMIT completed(run);
                                               return true;
                                           });
}

} // namespace Client
} // namespace QtOpenAi
