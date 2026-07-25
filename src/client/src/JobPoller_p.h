// SPDX-License-Identifier: MIT
#pragma once

// Private data for JobPoller and its subclasses. A concrete poller derives its
// own XxxPollerPrivate from this and stores the last observed job there, so the
// job member stays out of the public header (Qt d-pointer convention). The
// base's virtual destructor lets the single QScopedPointer<JobPollerPrivate> own
// the derived Private safely. Not installed / not part of the public API.

#include <QtCore/QPointer>
#include <QtCore/QString>

class QTimer;

namespace QtOpenAi {
namespace Client {

class Client;

class JobPollerPrivate
{
public:
    virtual ~JobPollerPrivate() = default;

    QPointer<Client> client;
    QString jobId;
    int intervalMs = 2000;
    bool polling = false;
    bool finished = false;
    bool autoDelete = true;
    QTimer *timer = nullptr; // single-shot; re-armed after each response
};

} // namespace Client
} // namespace QtOpenAi
