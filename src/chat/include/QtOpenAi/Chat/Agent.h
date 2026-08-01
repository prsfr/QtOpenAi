// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Chat/GlobalChat.h>
#include <QtOpenAi/Chat/Transcript.h>
#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Core/Message.h>
#include <QtOpenAi/Core/ToolCall.h>

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

#include <functional>

namespace QtOpenAi {
namespace Client {
class Client;
class ToolRegistry;
} // namespace Client

namespace Chat {

class AgentPrivate;

// Drives chat → tool calls → tool results → chat until there is an answer.
//
// That loop is the same in every application that uses tools, and
// examples/tool_loop.cpp is what writing it out looks like: a recursive lambda
// over replies, with the transcript, the dispatch and the termination condition
// all tangled together. This owns it.
//
//     Agent agent(&client, &registry);
//     agent.setModel("gpt-4o-mini");
//     connect(&agent, &Agent::finished, this, &Ui::showAnswer);
//     agent.run("What is the weather in Berlin?");
//
// The conversation accumulates in a Transcript, so an agent picks up where the
// last run left off and a trim policy keeps it inside the window.
//
// **A loop that talks to a model needs guards, and they are not optional
// extras.** `maxIterations` stops a model that keeps calling tools instead of
// answering; `timeoutMs` stops a run that stops making progress at all; and an
// approval callback lets the application refuse a call before it happens --
// declining is reported back to the model as the tool's result, so it can say
// so rather than hang.
class QTOPENAI_CHAT_EXPORT Agent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(bool streaming READ isStreaming WRITE setStreaming)
    Q_PROPERTY(int maxIterations READ maxIterations WRITE setMaxIterations)
public:
    // Decides whether a tool call may run. Returning false answers the model
    // with a refusal instead of a result.
    using ApprovalCallback = std::function<bool(const Core::ToolCall &call)>;

    // Neither the client nor the registry is owned; both must outlive the agent.
    // A null registry is allowed -- the agent then simply never has tools to
    // advertise, which is a plain chat loop.
    Agent(Client::Client *client, Client::ToolRegistry *tools, QObject *parent = nullptr);
    ~Agent() override;

    Client::Client *client() const;
    Client::ToolRegistry *tools() const;

    QString model() const;
    void setModel(const QString &model);

    // The conversation this agent is having. Replaceable, so a saved one can be
    // resumed.
    Transcript transcript() const;
    void setTranscript(const Transcript &transcript);

    QString systemPrompt() const;
    void setSystemPrompt(const QString &prompt);

    TrimPolicy trimPolicy() const;
    void setTrimPolicy(const TrimPolicy &policy);

    // Stream the answer, so contentDelta() arrives as it is written. The loop is
    // otherwise identical.
    bool isStreaming() const;
    void setStreaming(bool streaming);

    // How many times tools may be dispatched before the run is abandoned. Zero
    // means no limit, which is a decision rather than a default -- the default
    // is a small number.
    int maxIterations() const;
    void setMaxIterations(int iterations);

    // Wall-clock limit for a whole run, in milliseconds. Zero means none.
    int timeoutMs() const;
    void setTimeoutMs(int timeoutMs);

    void setApprovalCallback(ApprovalCallback callback);

    bool isRunning() const;

public Q_SLOTS:
    // Append the prompt and drive the loop. Returns false if a run is already in
    // progress -- an agent has one conversation at a time.
    bool run(const QString &prompt);
    // Continue from the transcript as it stands, with no new prompt.
    bool resume();
    // Abandon the run. failed() is emitted with a Network error saying so.
    void cancel();

Q_SIGNALS:
    // Every assistant turn, including the ones that only asked for tools.
    void assistantMessage(const QtOpenAi::Core::Message &message);
    // Streaming only: the answer as it is written.
    void contentDelta(const QString &text);
    void toolInvoked(const QString &name, const QString &result);
    // The approval callback refused this call; the model was told so.
    void toolRejected(const QString &name);
    // The run reached an answer.
    void finished(const QtOpenAi::Core::Message &message);
    void failed(const QtOpenAi::Client::ClientError &error);

    void modelChanged(const QString &model);
    void runningChanged(bool running);

private:
    Q_DECLARE_PRIVATE(Agent)
    QScopedPointer<AgentPrivate> d_ptr;
};

} // namespace Chat
} // namespace QtOpenAi
