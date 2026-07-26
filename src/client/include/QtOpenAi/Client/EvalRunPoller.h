// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/JobPoller.h>
#include <QtOpenAi/Core/EvalRun.h>

namespace QtOpenAi {
namespace Client {

class EvalRunPollerPrivate;

// A signal-based poll-until-terminal helper for eval runs.
//
// Scoring an eval's items is asynchronous, so this repeatedly issues GET
// /evals/{eval_id}/runs/{run_id} on a timer (via the owning Client) and reports
// every observed state — including the growing result counts — through
// progressed(). It stops once the run becomes terminal, emitting completed(), or
// when a request fails, emitting JobPoller::failed(). Created by
// Client::pollEvalRun(); auto-deletes after it stops unless disabled.
//
// Unlike the other pollers this one needs two ids: JobPoller carries the run id
// as its jobId(), and the owning eval's id is kept alongside it.
class QTOPENAI_CLIENT_EXPORT EvalRunPoller : public JobPoller
{
    Q_OBJECT
public:
    // The polled run's id — jobId() spelled the way this endpoint spells it.
    QString runId() const { return jobId(); }

    // The eval the polled run belongs to.
    QString evalId() const;

    // The most recently observed run state.
    Core::EvalRun run() const;

Q_SIGNALS:
    // Emitted after every successful poll with the current state, including the
    // terminal one.
    void progressed(const QtOpenAi::Core::EvalRun &run);
    // Emitted once when the run reaches a terminal state (Completed, Failed or
    // Canceled).
    void completed(const QtOpenAi::Core::EvalRun &run);

private:
    friend class Client;
    EvalRunPoller(Client *client, QString evalId, QString runId, int intervalMs,
                  QObject *parent = nullptr);

    void requestPoll() override;

    Q_DECLARE_PRIVATE(EvalRunPoller)
};

} // namespace Client
} // namespace QtOpenAi
