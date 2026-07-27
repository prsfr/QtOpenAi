// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/JobPoller.h>
#include <QtOpenAi/Core/Run.h>

namespace QtOpenAi {
namespace Client {

class RunPollerPrivate;

// A signal-based poll-until-terminal helper for assistant runs.
//
// It repeatedly issues GET /threads/{thread_id}/runs/{run_id} on a timer (via
// the owning Client) and reports every observed state through progressed().
// Created by Client::pollRun(); auto-deletes after it stops unless disabled.
//
// A run has two ways of stopping, and this poller stops for both:
//   * a terminal state (completed, failed, cancelled, incomplete, expired) —
//     emits completed();
//   * `requires_action` — the run is parked until the client answers the tool
//     calls, so waiting further would spin forever. It emits requiresAction()
//     instead. Submit the outputs with Client::submitToolOutputs() and start a
//     fresh poller on the same run to follow the rest of it.
//
// Like every poller, a request failure ends it with JobPoller::failed().
//
// Unlike the other pollers this one needs two ids: JobPoller carries the run id
// as its jobId(), and the owning thread's id is kept alongside it.
class QTOPENAI_CLIENT_EXPORT RunPoller : public JobPoller
{
    Q_OBJECT
public:
    // The polled run's id — jobId() spelled the way this endpoint spells it.
    QString runId() const { return jobId(); }

    // The thread the polled run belongs to.
    QString threadId() const;

    // The most recently observed run state.
    Core::Run run() const;

Q_SIGNALS:
    // Emitted after every successful poll with the current state, including the
    // one that stops the poll.
    void progressed(const QtOpenAi::Core::Run &run);
    // Emitted once when the run reaches a terminal state.
    void completed(const QtOpenAi::Core::Run &run);
    // Emitted once when the run parks on `requires_action`, carrying the run
    // whose requiredToolCalls() are waiting to be answered.
    void requiresAction(const QtOpenAi::Core::Run &run);

private:
    friend class Client;
    RunPoller(Client *client, QString threadId, QString runId, int intervalMs,
              QObject *parent = nullptr);

    void requestPoll() override;

    Q_DECLARE_PRIVATE(RunPoller)
};

} // namespace Client
} // namespace QtOpenAi
