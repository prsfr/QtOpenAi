// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Client {

class Client;
class JobPollerPrivate;

// Shared engine for the poll-until-terminal helpers.
//
// Several endpoints hand back a job that finishes asynchronously — a Sora render
// (/videos), a batch run (/batches). The polling itself is identical in each
// case: re-issue a GET on a timer, report every observed state, stop on the
// first terminal one, stop on a request error. Only the job type and its typed
// signals differ.
//
// This base owns the timer, the lifecycle flags and the auto-delete policy. A
// subclass adds its typed job getter plus typed progressed()/completed()
// signals, and implements requestPoll() to issue one typed GET and hand the
// reply to trackPoll().
class QTOPENAI_CLIENT_EXPORT JobPoller : public QObject
{
    Q_OBJECT
public:
    ~JobPoller() override;

    // Id of the polled job (a video id, a batch id, ...).
    QString jobId() const;

    // Delay between successive polls in milliseconds (default 2000).
    int pollIntervalMs() const;
    void setPollIntervalMs(int intervalMs);

    bool isPolling() const;
    bool isFinished() const;

    // Delete the poller once it stops (default true).
    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    // Begin polling (issues the first request immediately). No-op once the
    // poller has finished.
    void start();
    // Stop polling without emitting any terminal signal.
    void stop();

Q_SIGNALS:
    // Emitted once when a poll request itself fails (network/HTTP/parse). A job
    // that merely *reports* a failed state is a normal terminal outcome and
    // arrives through the subclass's completed() signal instead.
    void failed(const QtOpenAi::Client::ClientError &error);

protected:
    // Constructed by subclasses with their own JobPollerPrivate subclass, so the
    // last observed job stays out of this header (Qt d-pointer convention).
    JobPoller(JobPollerPrivate &dd, Client *client, QString jobId, int intervalMs,
              QObject *parent = nullptr);

    // The owning client; null once it has been destroyed.
    Client *client() const;

    // Issue exactly one poll request and pass the reply to trackPoll(). Called
    // by start() and by the interval timer.
    virtual void requestPoll() = 0;

    // Wire a typed reply returned by requestPoll(). A request failure ends the
    // poll and emits failed(); every successful state is handed to `onState`,
    // which stores it and emits the subclass's typed signals. `onState` returns
    // true when the poll is over (it has called finish() and emitted
    // completed()); otherwise the next poll is scheduled.
    template <typename Reply, typename Job, typename Handler>
    void trackPoll(Reply *reply, Handler onState)
    {
        connect(reply, &Reply::finished, this, [this, onState](const Job &job) {
            if (!isPolling())
                return;
            if (!onState(job))
                scheduleNextPoll();
        });
        connect(reply, &Reply::failed, this, [this](const ClientError &error) {
            if (!isPolling())
                return;
            finish();
            Q_EMIT failed(error);
        });
    }

    // The whole of requestPoll() for a poller that simply runs until its job
    // goes terminal, which is all of them but RunPoller: store the observed job
    // in `slot`, report it through `progressed`, and on the first terminal state
    // stop and report it through `completed`.
    //
    // Four pollers spelled that same six-line lambda out, differing in nothing
    // but the three names -- so the names are what they pass. `progressed` and
    // `completed` are pointers to the subclass's own signals; calling one
    // through a member pointer emits it exactly as Q_EMIT does, Q_EMIT being a
    // no-op marker. `slot` is a member of the subclass's JobPollerPrivate,
    // which this header cannot name but a template parameter can.
    template <typename Reply, typename Job, typename Private, typename Poller>
    void trackTerminalPoll(Reply *reply, Private *d, Job Private::*slot,
                           void (Poller::*progressed)(const Job &),
                           void (Poller::*completed)(const Job &))
    {
        trackPoll<Reply, Job>(reply, [this, d, slot, progressed, completed](const Job &job) {
            d->*slot = job;
            Poller *poller = static_cast<Poller *>(this);
            (poller->*progressed)(job);
            if (!job.isTerminal())
                return false;
            finish();
            (poller->*completed)(job);
            return true;
        });
    }

    // Mark polling as finished and honour the auto-delete policy. Subclasses
    // call this from `onState` before emitting their completed() signal.
    void finish();

    // Re-arm the interval timer for the next poll.
    void scheduleNextPoll();

    QScopedPointer<JobPollerPrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(JobPoller)
};

} // namespace Client
} // namespace QtOpenAi
