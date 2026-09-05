// SPDX-License-Identifier: MIT
#include "QtOpenAi/Storage/Autosave.h"

#include "QtOpenAi/Client/MetricsCollector.h"
#include "QtOpenAi/Storage/Store.h"

#include <QtCore/QPointer>
#include <QtCore/QTimer>

namespace QtOpenAi {
namespace Storage {

class AutosavePrivate
{
public:
    Store *store = nullptr;
    QTimer timer;

    QString conversationId;
    std::function<Chat::Transcript()> conversationSource;

    QString metricsId;
    QPointer<Client::MetricsCollector> collector;

    bool enabled = true;
    bool dirty = false;
};

Autosave::Autosave(Store *store, QObject *parent)
    : QObject(parent)
    , d_ptr(new AutosavePrivate)
{
    Q_D(Autosave);
    d->store = store;
    // Single-shot and started only by the first touch after a save: the timer
    // is the interval since something changed, not a heartbeat that wakes a
    // sleeping application to write nothing.
    d->timer.setSingleShot(true);
    d->timer.setInterval(5000);
    connect(&d->timer, &QTimer::timeout, this, [this] { flush(); });
}

Autosave::~Autosave() = default;

Store *Autosave::store() const
{
    Q_D(const Autosave);
    return d->store;
}

void Autosave::setConversation(const QString &id, std::function<Chat::Transcript()> source)
{
    Q_D(Autosave);
    d->conversationId = id;
    d->conversationSource = std::move(source);
}

QString Autosave::conversationId() const
{
    Q_D(const Autosave);
    return d->conversationId;
}

void Autosave::setMetrics(const QString &id, Client::MetricsCollector *collector)
{
    Q_D(Autosave);
    if (d->collector)
        disconnect(d->collector, nullptr, this, nullptr);
    d->metricsId = id;
    d->collector = collector;
    if (collector) {
        connect(collector, &Client::MetricsCollector::requestRecorded, this, &Autosave::touch);
        connect(collector, &Client::MetricsCollector::usageRecorded, this, &Autosave::touch);
    }
}

QString Autosave::metricsId() const
{
    Q_D(const Autosave);
    return d->metricsId;
}

int Autosave::intervalMs() const
{
    Q_D(const Autosave);
    return d->timer.interval();
}

void Autosave::setIntervalMs(int ms)
{
    Q_D(Autosave);
    d->timer.setInterval(qMax(0, ms));
}

bool Autosave::isEnabled() const
{
    Q_D(const Autosave);
    return d->enabled;
}

void Autosave::setEnabled(bool enabled)
{
    Q_D(Autosave);
    if (d->enabled == enabled)
        return;
    d->enabled = enabled;
    if (!enabled) {
        d->timer.stop();
        return;
    }
    // Re-enabling saves what happened while it was off; the change is still
    // recorded, only the writing of it was suspended.
    if (d->dirty)
        d->timer.start();
}

bool Autosave::isDirty() const
{
    Q_D(const Autosave);
    return d->dirty;
}

void Autosave::touch()
{
    Q_D(Autosave);
    if (!d->dirty) {
        d->dirty = true;
        Q_EMIT dirtyChanged();
    }
    if (!d->enabled)
        return;
    if (d->timer.interval() == 0) {
        flush();
        return;
    }
    // Already running means a save is due sooner than restarting would make it,
    // and restarting on every change is how a busy conversation never saves.
    if (!d->timer.isActive())
        d->timer.start();
}

bool Autosave::flush()
{
    Q_D(Autosave);
    d->timer.stop();
    if (!d->dirty)
        return true;
    if (!d->store) {
        Q_EMIT failed(QStringLiteral("Autosave: no store."));
        return false;
    }

    // The conversation and the metrics snapshot are one batch: they are the
    // two writes of every interval, and a backend that can group them commits
    // once rather than twice for each of them.
    Store::Batch batch(d->store);

    bool ok = true;
    if (!d->conversationId.isEmpty() && d->conversationSource) {
        if (!d->store->saveConversation(d->conversationId, d->conversationSource())) {
            ok = false;
            Q_EMIT failed(d->store->lastError());
        }
    }
    if (ok && !d->metricsId.isEmpty() && d->collector) {
        if (!d->store->saveMetrics(d->metricsId, d->collector->snapshot())) {
            ok = false;
            Q_EMIT failed(d->store->lastError());
        }
    }

    if (!ok) {
        // Still dirty: what failed has not been written, and the next touch
        // should try again rather than assume it is safe on disk.
        batch.abort();
        return false;
    }
    // A store that would not start a batch has written the two above anyway,
    // ungrouped; only one it did start can still fail to land.
    if (batch.isActive() && !batch.commit()) {
        Q_EMIT failed(d->store->lastError());
        return false;
    }

    d->dirty = false;
    Q_EMIT dirtyChanged();
    Q_EMIT saved();
    return true;
}

} // namespace Storage
} // namespace QtOpenAi
