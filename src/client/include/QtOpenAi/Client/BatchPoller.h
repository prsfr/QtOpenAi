// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/JobPoller.h>
#include <QtOpenAi/Core/Batch.h>

namespace QtOpenAi {
namespace Client {

class BatchPollerPrivate;

// A signal-based poll-until-terminal helper for batch jobs.
//
// A batch runs for up to its completion window, so BatchPoller repeatedly issues
// GET /batches/{id} on a timer (via the owning Client) and reports every
// observed state — including the growing request counts — through progressed().
// It stops once the batch becomes terminal, emitting completed(), or when a
// request fails, emitting JobPoller::failed(). Created by Client::pollBatch();
// auto-deletes after it stops unless disabled.
//
// A `cancelling` batch is not terminal: polling continues until it settles on
// `cancelled`.
class QTOPENAI_CLIENT_EXPORT BatchPoller : public JobPoller
{
    Q_OBJECT
public:
    // The polled batch's id — jobId() spelled the way this endpoint spells it.
    QString batchId() const { return jobId(); }

    // The most recently observed batch state.
    Core::Batch batch() const;

Q_SIGNALS:
    // Emitted after every successful poll with the current state, including the
    // terminal one.
    void progressed(const QtOpenAi::Core::Batch &batch);
    // Emitted once when the batch reaches a terminal state (Completed, Failed,
    // Expired or Cancelled).
    void completed(const QtOpenAi::Core::Batch &batch);

private:
    friend class Client;
    BatchPoller(Client *client, QString batchId, int intervalMs, QObject *parent = nullptr);

    void requestPoll() override;

    Q_DECLARE_PRIVATE(BatchPoller)
};

} // namespace Client
} // namespace QtOpenAi
