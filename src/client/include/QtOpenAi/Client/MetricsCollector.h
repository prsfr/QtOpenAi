// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/Metrics.h>
#include <QtOpenAi/Core/ModelCatalog.h>

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

namespace QtOpenAi {
namespace Client {

class Client;
class RestReplyBase;
class MetricsCollectorPrivate;

// What the client is costing you, and how it is behaving.
//
// Attach one to a Client and every request it makes is timed and counted --
// duration, outcome, HTTP status, retries, and the rate-limit headroom the
// provider reported:
//
//     MetricsCollector metrics;
//     metrics.attach(&client);
//     ...
//     metrics.snapshot().averageDurationMs();
//
// Tokens and cost need one more thing. A reply is generic; only the *typed*
// response knows which model answered and what it spent, so `observe()` hooks
// that reply's own finished signal:
//
//     metrics.observe(client.createChatCompletion(request));
//
// It compiles for any reply whose response reports `model()` and `usage()`, and
// fails to compile for one that does not -- which is the right answer for a
// file upload.
//
// **Time to first token** is what a user perceives as latency, and it is only
// meaningful for a stream. Streamed replies announce their first fragment; the
// collector finds that signal through the meta-object rather than by naming
// each streaming reply type, so a streaming endpoint added later is covered on
// the day it is added.
//
// Cost comes from Core::ModelCatalog's prices, read at the moment each request
// is recorded. A model the catalog has no price for contributes zero, which is
// an honest "unknown" rather than "free". `setCatalog()` takes a corrected
// table.
//
// **Nothing is paid for when nothing is attached.** The Client announces each
// reply through an ordinary signal, and an unconnected signal costs a
// comparison.
class QTOPENAI_CLIENT_EXPORT MetricsCollector : public QObject
{
    Q_OBJECT
public:
    explicit MetricsCollector(QObject *parent = nullptr);
    ~MetricsCollector() override;

    // Record every request this client makes from now on. Attaching twice is
    // harmless; detaching stops the recording without discarding what was
    // already recorded.
    void attach(Client *client);
    void detach(Client *client);

    // Record tokens and cost from a typed reply, which is the only place the
    // model and its usage appear. Returns the reply, so it wraps a call.
    template <typename Reply>
    Reply *observe(Reply *reply)
    {
        if (reply)
            connectFinished(reply, &Reply::finished);
        return reply;
    }

    // The same, for a caller holding the numbers directly.
    void recordUsage(const QString &model, const Core::Usage &usage);
    void recordRequest(const RequestMetrics &metrics);

    MetricsSnapshot snapshot() const;
    ModelMetrics metrics(const QString &model) const;

    // Pricing. Defaults to Core::ModelCatalog::shared().
    Core::ModelCatalog catalog() const;
    void setCatalog(const Core::ModelCatalog &catalog);

    void reset();

Q_SIGNALS:
    // One completed request, whatever its outcome.
    void requestRecorded(const QtOpenAi::Client::RequestMetrics &metrics);
    // Tokens attributed to a model -- the point at which cost changes.
    void usageRecorded(const QString &model, const QtOpenAi::Core::Usage &usage);

private:
    // Deduces the response type from the reply's own finished signal, so
    // observe() fails to compile for a reply whose response has no model or
    // usage -- which is the right answer for a file upload.
    template <typename Reply, typename Response>
    void connectFinished(Reply *reply, void (Reply::*finished)(const Response &))
    {
        QObject::connect(reply, finished, this, [this](const Response &response) {
            recordUsage(response.model(), response.usage());
        });
    }

    // Reached from Client::replyCreated.
    void observeReply(QObject *reply);

    // Reached through meta-object connections to whichever of `contentDelta`,
    // `failed` and `done` the reply happens to have. Not part of the API.
    Q_INVOKABLE void markFirstToken();
    Q_INVOKABLE void markFailed();
    Q_INVOKABLE void markDone();

    Q_DECLARE_PRIVATE(MetricsCollector)
    QScopedPointer<MetricsCollectorPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
