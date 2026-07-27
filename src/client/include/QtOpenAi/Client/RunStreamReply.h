// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>
#include <QtOpenAi/Core/Run.h>
#include <QtOpenAi/Core/ThreadMessage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QObject>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class RunStreamReplyPrivate;

// An asynchronous handle for a streamed (`stream: true`) assistant run.
//
// The run stream is a sequence of events whose payloads are ordinary Assistants
// objects; this reply routes them by their `object` field. Every event is
// surfaced through event(object, data); the two that matter for a UI get their
// own signals — messageDelta() for incremental assistant text, and runChanged()
// for each new run state.
//
// The stream ends the way the run does. A terminal run state emits finished()
// with the final Run; a run that parks on `requires_action` emits
// requiresAction() and then finished() as well, because the request itself is
// over — answer the tool calls with Client::submitToolOutputsStream(), which
// resumes the run as a new stream (or submitToolOutputs() to continue without
// one). An HTTP error, or a stream that stops before any terminal state, emits
// failed(). Both precede done(), after which the object deletes itself unless
// disabled.
class QTOPENAI_CLIENT_EXPORT RunStreamReply : public QObject
{
    Q_OBJECT
public:
    ~RunStreamReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    // The last run state seen (default-constructed until the first event).
    Core::Run run() const;
    ClientError error() const;

    RateLimit rateLimit() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    // Every streamed event, keyed by the payload's `object` (e.g. "thread.run",
    // "thread.message.delta", "thread.run.step").
    void event(const QString &object, const QJsonObject &data);
    // Incremental assistant text from a `thread.message.delta` event.
    void messageDelta(const QString &text);
    // A completed message from a `thread.message` event.
    void messageCompleted(const QtOpenAi::Core::ThreadMessage &message);
    // Each new state of the run.
    void runChanged(const QtOpenAi::Core::Run &run);
    // The run is waiting for the outputs of its requiredToolCalls().
    void requiresAction(const QtOpenAi::Core::Run &run);
    void finished(const QtOpenAi::Core::Run &run);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();

private:
    friend class Client;
    explicit RunStreamReply(QNetworkReply *reply, QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(RunStreamReply)
    QScopedPointer<RunStreamReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
