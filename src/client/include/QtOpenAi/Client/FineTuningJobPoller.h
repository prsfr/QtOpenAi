// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/JobPoller.h>
#include <QtOpenAi/Core/FineTuningJob.h>

namespace QtOpenAi {
namespace Client {

class FineTuningJobPollerPrivate;

// A signal-based poll-until-terminal helper for fine-tuning jobs.
//
// Training runs for minutes to hours, so this repeatedly issues GET
// /fine_tuning/jobs/{id} on a timer (via the owning Client) and reports every
// observed state through progressed(). It stops once the job becomes terminal,
// emitting completed(), or when a request fails, emitting JobPoller::failed().
// Created by Client::pollFineTuningJob(); auto-deletes after it stops unless
// disabled.
//
// A paused job is not terminal, so polling continues across a pause/resume.
class QTOPENAI_CLIENT_EXPORT FineTuningJobPoller : public JobPoller
{
    Q_OBJECT
public:
    // The polled job's id — jobId() is already the endpoint's own spelling.
    Core::FineTuningJob job() const;

Q_SIGNALS:
    // Emitted after every successful poll with the current state, including the
    // terminal one.
    void progressed(const QtOpenAi::Core::FineTuningJob &job);
    // Emitted once when the job reaches a terminal state (Succeeded, Failed or
    // Cancelled).
    void completed(const QtOpenAi::Core::FineTuningJob &job);

private:
    friend class Client;
    FineTuningJobPoller(Client *client, QString jobId, int intervalMs, QObject *parent = nullptr);

    void requestPoll() override;

    Q_DECLARE_PRIVATE(FineTuningJobPoller)
};

} // namespace Client
} // namespace QtOpenAi
