// SPDX-License-Identifier: MIT
#include "QtOpenAi/Chat/Agent.h"

#include <QtOpenAi/Client/ChatCompletionReply.h>
#include <QtOpenAi/Client/ChatCompletionStreamReply.h>
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Client/StreamReplyBase.h>
#include <QtOpenAi/Client/ToolRegistry.h>

#include "JsonHelpers_p.h"

#include <QtCore/QJsonObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>

namespace QtOpenAi {
namespace Chat {

namespace {

constexpr int kDefaultMaxIterations = 8;

// What the model is told when the application refuses a call. It has to look
// like an ordinary tool result, or the model has no way to react to it.
QString refusalPayload(const QString &name)
{
    QJsonObject error;
    error.insert(QStringLiteral("error"),
                 QStringLiteral("the tool '%1' was not permitted to run").arg(name));
    return Core::detail::compactJsonText(error);
}

} // namespace

class AgentPrivate
{
public:
    Agent *q = nullptr;
    QPointer<Client::Client> client;
    QPointer<Client::ToolRegistry> tools;

    QString model;
    Transcript transcript;
    bool streaming = false;
    int maxIterations = kDefaultMaxIterations;
    int timeoutMs = 0;
    Agent::ApprovalCallback approval;

    bool running = false;
    int iteration = 0;
    // The request in flight, held as the two kinds it can actually be rather
    // than as a bare QObject. abort() is a plain public member on both bases,
    // not a slot and not Q_INVOKABLE, so it is absent from the meta-object:
    // reaching it by name left both stop paths doing nothing but printing a
    // warning. Two typed pointers cost one branch and let the compiler check
    // the call. Only one is ever non-null.
    QPointer<Client::RestReplyBase> onceReply;
    QPointer<Client::StreamReplyBase> streamReply;
    QTimer deadline;

    void setRunning(bool value)
    {
        if (running == value)
            return;
        running = value;
        Q_EMIT q->runningChanged(value);
    }

    void fail(const QString &message,
              Client::ClientError::Kind kind = Client::ClientError::Kind::Network)
    {
        finish();
        Q_EMIT q->failed(Client::ClientError(kind, message));
    }

    // Lets go of the reply without touching the transfer: the right thing when
    // it has already settled and handed us its answer.
    void forgetReply()
    {
        onceReply = nullptr;
        streamReply = nullptr;
    }

    // Stops the transfer as well, which is what cancel() and the deadline want:
    // otherwise the provider goes on generating -- and billing -- an answer
    // that nothing will read.
    //
    // Our connections go first. abort() makes the reply fail, and that failure
    // is no longer ours to report: the caller is about to emit a failed() of
    // its own saying why the run stopped, and without the disconnect one
    // cancel() would deliver two.
    void abortReply()
    {
        Client::RestReplyBase *once = onceReply.data();
        Client::StreamReplyBase *stream = streamReply.data();
        forgetReply();

        if (once) {
            QObject::disconnect(once, nullptr, q, nullptr);
            once->abort();
        }
        if (stream) {
            QObject::disconnect(stream, nullptr, q, nullptr);
            stream->abort();
        }
    }

    void finish()
    {
        deadline.stop();
        forgetReply();
        setRunning(false);
    }

    // One turn: send what the transcript says, then react to the answer.
    void step();
    void handleAnswer(const Core::ChatCompletionResponse &response);
};

void AgentPrivate::step()
{
    // `iteration` counts dispatches already made, so the limit is reached
    // rather than exceeded: maxIterations of 2 dispatches tools twice.
    if (maxIterations > 0 && iteration >= maxIterations) {
        fail(QStringLiteral("gave up after %1 tool iterations").arg(maxIterations),
             Client::ClientError::Kind::InvalidRequest);
        return;
    }
    if (!client) {
        fail(QStringLiteral("the agent has no client"), Client::ClientError::Kind::InvalidRequest);
        return;
    }

    Core::ChatCompletionRequest request = transcript.buildRequest(model);
    if (tools)
        request.setTools(tools->tools());

    const auto onFailed = [this](const Client::ClientError &error) {
        finish();
        Q_EMIT q->failed(error);
    };

    forgetReply();
    if (streaming) {
        Client::ChatCompletionStreamReply *stream = client->createChatCompletionStream(request);
        streamReply = stream;
        QObject::connect(stream, &Client::ChatCompletionStreamReply::contentDelta, q,
                         &Agent::contentDelta);
        QObject::connect(stream, &Client::ChatCompletionStreamReply::failed, q, onFailed);
        QObject::connect(
                stream, &Client::ChatCompletionStreamReply::finished, q,
                [this](const Core::ChatCompletionResponse &response) { handleAnswer(response); });
    } else {
        Client::ChatCompletionReply *once = client->createChatCompletion(request);
        onceReply = once;
        QObject::connect(once, &Client::ChatCompletionReply::failed, q, onFailed);
        QObject::connect(
                once, &Client::ChatCompletionReply::finished, q,
                [this](const Core::ChatCompletionResponse &response) { handleAnswer(response); });
    }
}

void AgentPrivate::handleAnswer(const Core::ChatCompletionResponse &response)
{
    // A cancelled run may still see its reply land; it is no longer ours.
    if (!running)
        return;
    forgetReply();

    const Core::Message message = response.firstMessage();
    transcript.addMessage(message);
    Q_EMIT q->assistantMessage(message);

    const QList<Core::ToolCall> calls = message.toolCalls();
    if (calls.isEmpty()) {
        // No tools asked for: this is the answer.
        finish();
        Q_EMIT q->finished(message);
        return;
    }

    // `running` can go false part-way through this loop, and the loop has to
    // notice. Three things in it call out to code the agent does not control --
    // the approval callback, the tool handler, and whatever is connected to
    // toolInvoked/toolRejected -- and any of them may legitimately call
    // cancel(). The deadline can fire here too, without any application code
    // at all: a tool that blocks on a nested event loop delivers the agent's
    // own timer while it waits, which is exactly what HttpTools' http_get
    // documents itself as doing. Without these checks a cancelled run keeps
    // invoking the remaining tools and then dispatches another turn, whose
    // answer arrives unowned and lands in whichever run comes next.
    for (const Core::ToolCall &call : calls) {
        if (!running)
            return;

        const QString name = call.function().name();
        if (approval && !approval(call)) {
            // Refusing has to look like a result, or the model cannot react.
            transcript.addMessage(Core::Message::toolResult(call.id(), refusalPayload(name)));
            Q_EMIT q->toolRejected(name);
            continue;
        }
        // Approving and cancelling are not exclusive: a callback that stops the
        // run and still returns true has approved a call that must no longer
        // happen, so this cannot wait for the next turn of the loop.
        if (!running)
            return;

        const Core::Message result
                = tools ? tools->invoke(call)
                        : Core::Message::toolResult(call.id(), refusalPayload(name));
        transcript.addMessage(result);
        Q_EMIT q->toolInvoked(name, result.content());
    }

    if (!running)
        return;

    ++iteration;
    step();
}

Agent::Agent(Client::Client *client, Client::ToolRegistry *tools, QObject *parent)
    : QObject(parent)
    , d_ptr(new AgentPrivate)
{
    Q_D(Agent);
    d->q = this;
    d->client = client;
    d->tools = tools;
    d->deadline.setSingleShot(true);
    connect(&d->deadline, &QTimer::timeout, this, [this] {
        Q_D(Agent);
        // A run that stopped making progress is worse than one that failed:
        // nothing will ever arrive to end it.
        d->abortReply();
        d->fail(QStringLiteral("the run exceeded %1 ms").arg(d->timeoutMs));
    });
}

Agent::~Agent() = default;

Client::Client *Agent::client() const
{
    Q_D(const Agent);
    return d->client.data();
}

Client::ToolRegistry *Agent::tools() const
{
    Q_D(const Agent);
    return d->tools.data();
}

QString Agent::model() const
{
    Q_D(const Agent);
    return d->model;
}

void Agent::setModel(const QString &model)
{
    Q_D(Agent);
    if (d->model == model)
        return;
    d->model = model;
    Q_EMIT modelChanged(model);
}

Transcript Agent::transcript() const
{
    Q_D(const Agent);
    return d->transcript;
}

void Agent::setTranscript(const Transcript &transcript)
{
    Q_D(Agent);
    d->transcript = transcript;
}

QString Agent::systemPrompt() const
{
    Q_D(const Agent);
    return d->transcript.systemPrompt();
}

void Agent::setSystemPrompt(const QString &prompt)
{
    Q_D(Agent);
    d->transcript.setSystemPrompt(prompt);
}

TrimPolicy Agent::trimPolicy() const
{
    Q_D(const Agent);
    return d->transcript.trimPolicy();
}

void Agent::setTrimPolicy(const TrimPolicy &policy)
{
    Q_D(Agent);
    d->transcript.setTrimPolicy(policy);
}

bool Agent::isStreaming() const
{
    Q_D(const Agent);
    return d->streaming;
}

void Agent::setStreaming(bool streaming)
{
    Q_D(Agent);
    d->streaming = streaming;
}

int Agent::maxIterations() const
{
    Q_D(const Agent);
    return d->maxIterations;
}

void Agent::setMaxIterations(int iterations)
{
    Q_D(Agent);
    d->maxIterations = qMax(0, iterations);
}

int Agent::timeoutMs() const
{
    Q_D(const Agent);
    return d->timeoutMs;
}

void Agent::setTimeoutMs(int timeoutMs)
{
    Q_D(Agent);
    d->timeoutMs = qMax(0, timeoutMs);
}

void Agent::setApprovalCallback(ApprovalCallback callback)
{
    Q_D(Agent);
    d->approval = std::move(callback);
}

bool Agent::isRunning() const
{
    Q_D(const Agent);
    return d->running;
}

bool Agent::run(const QString &prompt)
{
    Q_D(Agent);
    if (d->running)
        return false;
    d->transcript.addUserMessage(prompt);
    return resume();
}

bool Agent::resume()
{
    Q_D(Agent);
    if (d->running)
        return false;

    d->iteration = 0;
    d->setRunning(true);
    if (d->timeoutMs > 0)
        d->deadline.start(d->timeoutMs);
    d->step();
    return true;
}

void Agent::cancel()
{
    Q_D(Agent);
    if (!d->running)
        return;
    d->abortReply();
    d->fail(QStringLiteral("the run was cancelled"));
}

} // namespace Chat
} // namespace QtOpenAi
