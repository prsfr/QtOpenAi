// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/MetricsCollector.h"

#include "QtOpenAi/Client/Client.h"
#include "QtOpenAi/Client/RestReplyBase.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QMetaMethod>
#include <QtCore/QPointer>

namespace QtOpenAi {
namespace Client {

namespace {

// A reply's signal, by name. Every reply announces itself through `done`,
// `failed` and -- if it streams -- `contentDelta`, but they share no base
// class that says so: the streaming replies sit outside the retry machinery
// and are plain QObjects. Looking the signals up rather than naming each reply
// type is what lets one collector cover all ~70 of them, and cover the next one
// without being touched.
QMetaMethod signalNamed(const QMetaObject *meta, const char *name)
{
    for (int i = 0; i < meta->methodCount(); ++i) {
        const QMetaMethod method = meta->method(i);
        if (method.methodType() == QMetaMethod::Signal && method.name() == name)
            return method;
    }
    return {};
}

} // namespace

class MetricsCollectorPrivate
{
public:
    struct Pending
    {
        qint64 startedMs = 0;
        qint64 firstTokenMs = -1;
        bool failed = false;
    };

    // One monotonic clock for the collector: every timestamp is an offset from
    // it, so the arithmetic survives the system clock moving.
    QElapsedTimer clock;
    QHash<const QObject *, Pending> pending;
    MetricsSnapshot snapshot;
    Core::ModelCatalog catalog = Core::ModelCatalog::shared();
    QList<QPointer<Client>> clients;
};

MetricsCollector::MetricsCollector(QObject *parent)
    : QObject(parent)
    , d_ptr(new MetricsCollectorPrivate)
{
    Q_D(MetricsCollector);
    d->clock.start();
}

MetricsCollector::~MetricsCollector() = default;

void MetricsCollector::attach(Client *client)
{
    Q_D(MetricsCollector);
    if (!client || d->clients.contains(client))
        return;
    d->clients.append(client);
    connect(client, &Client::replyCreated, this, &MetricsCollector::observeReply);
}

void MetricsCollector::detach(Client *client)
{
    Q_D(MetricsCollector);
    if (!client)
        return;
    disconnect(client, &Client::replyCreated, this, &MetricsCollector::observeReply);
    d->clients.removeAll(client);
}

void MetricsCollector::observeReply(QObject *reply)
{
    Q_D(MetricsCollector);
    if (!reply || d->pending.contains(reply))
        return;

    d->pending.insert(reply, {d->clock.elapsed(), -1, false});

    // A reply destroyed without finishing leaves nothing behind.
    connect(reply, &QObject::destroyed, this, [this](QObject *object) {
        Q_D(MetricsCollector);
        d->pending.remove(object);
    });

    const QMetaObject *meta = reply->metaObject();
    const QMetaObject &self = MetricsCollector::staticMetaObject;
    const auto bind = [&](const char *signalName, const char *slotName) {
        const QMetaMethod signal = signalNamed(meta, signalName);
        if (signal.isValid())
            QObject::connect(reply, signal, this, self.method(self.indexOfMethod(slotName)));
    };

    bind("contentDelta", "markFirstToken()");
    bind("failed", "markFailed()");
    bind("done", "markDone()");
}

void MetricsCollector::markFirstToken()
{
    Q_D(MetricsCollector);
    const auto it = d->pending.find(sender());
    // Only the first fragment says anything; the rest are the stream running.
    if (it != d->pending.end() && it->firstTokenMs < 0)
        it->firstTokenMs = d->clock.elapsed();
}

void MetricsCollector::markFailed()
{
    Q_D(MetricsCollector);
    const auto it = d->pending.find(sender());
    if (it != d->pending.end())
        it->failed = true;
}

void MetricsCollector::markDone()
{
    Q_D(MetricsCollector);
    QObject *reply = sender();
    const auto it = d->pending.constFind(reply);
    if (it == d->pending.constEnd())
        return;

    RequestMetrics metrics;
    metrics.durationMs = d->clock.elapsed() - it->startedMs;
    metrics.timeToFirstTokenMs = it->firstTokenMs < 0 ? -1 : it->firstTokenMs - it->startedMs;
    metrics.ok = !it->failed;

    // The retrying replies know more about themselves than a stream does.
    if (const RestReplyBase *rest = qobject_cast<RestReplyBase *>(reply)) {
        metrics.ok = rest->isSuccess();
        metrics.retryCount = rest->retryCount();
        metrics.rateLimit = rest->rateLimit();
        metrics.errorKind = rest->error().kind();
        metrics.httpStatus = rest->error().httpStatus();
    } else if (it->failed) {
        // A stream that broke reports no status of its own; 0 is the bucket for
        // "no response to read one from".
        metrics.errorKind = ClientError::Kind::Network;
    }

    d->pending.remove(reply);
    recordRequest(metrics);
}

void MetricsCollector::recordRequest(const RequestMetrics &metrics)
{
    Q_D(MetricsCollector);

    ++d->snapshot.requests;
    if (metrics.ok) {
        ++d->snapshot.successes;
    } else {
        ++d->snapshot.failures;
        // Status 0 is the bucket for requests that never got a response at all.
        ++d->snapshot.failuresByStatus[metrics.httpStatus];
    }

    d->snapshot.totalDurationMs += metrics.durationMs;
    d->snapshot.slowestDurationMs = qMax(d->snapshot.slowestDurationMs, metrics.durationMs);
    if (metrics.timeToFirstTokenMs >= 0) {
        ++d->snapshot.streamedRequests;
        d->snapshot.totalTimeToFirstTokenMs += metrics.timeToFirstTokenMs;
    }
    if (metrics.rateLimit.isValid())
        d->snapshot.rateLimit = metrics.rateLimit;

    // A caller recording by hand may already know the model and its usage; one
    // recorded from a reply does not, and says so with an empty name.
    if (!metrics.model.isEmpty())
        recordUsage(metrics.model, metrics.usage);

    Q_EMIT requestRecorded(metrics);
}

void MetricsCollector::recordUsage(const QString &model, const Core::Usage &usage)
{
    Q_D(MetricsCollector);

    const Core::ModelInfo info = d->catalog.model(model);

    ModelMetrics entry = d->snapshot.models.value(model);
    ++entry.requests;
    entry.promptTokens += usage.promptTokens();
    entry.completionTokens += usage.completionTokens();
    // A response that reports no total is still worth the sum of its parts.
    entry.totalTokens += usage.totalTokens() > 0 ? usage.totalTokens()
                                                 : usage.promptTokens() + usage.completionTokens();
    // Prices are per million tokens, which is the unit the price list uses. An
    // unpriced model adds nothing rather than a guess.
    entry.cost += usage.promptTokens() * info.inputPrice() / 1e6
                  + usage.completionTokens() * info.outputPrice() / 1e6;
    d->snapshot.models.insert(model, entry);

    Q_EMIT usageRecorded(model, usage);
}

MetricsSnapshot MetricsCollector::snapshot() const
{
    Q_D(const MetricsCollector);
    return d->snapshot;
}

ModelMetrics MetricsCollector::metrics(const QString &model) const
{
    Q_D(const MetricsCollector);
    return d->snapshot.models.value(model);
}

Core::ModelCatalog MetricsCollector::catalog() const
{
    Q_D(const MetricsCollector);
    return d->catalog;
}

void MetricsCollector::setCatalog(const Core::ModelCatalog &catalog)
{
    Q_D(MetricsCollector);
    d->catalog = catalog;
}

void MetricsCollector::reset()
{
    Q_D(MetricsCollector);
    d->snapshot = MetricsSnapshot();
    // Requests already in flight are still timed; only what was counted goes.
}

} // namespace Client
} // namespace QtOpenAi
