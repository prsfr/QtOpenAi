// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/Enums.h>
#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>
#include <QtOpenAi/Core/ToolCall.h>
#include <QtOpenAi/Core/Usage.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {

// The result of one tool call, handed back to a run that is waiting for it
// (POST /threads/{id}/runs/{run_id}/submit_tool_outputs). `output` is whatever
// the local handler produced, as a string — JSON if the tool returns
// structured data.
struct QTOPENAI_CORE_EXPORT ToolOutput
{
    QString toolCallId;
    QString output;

    QJsonObject toJson() const;
    static ToolOutput fromJson(const QJsonObject &json);

    bool operator==(const ToolOutput &other) const
    {
        return toolCallId == other.toolCallId && output == other.output;
    }
    bool operator!=(const ToolOutput &other) const { return !(*this == other); }
};

class RunData;

// One execution of an assistant against a thread (POST /threads/{id}/runs,
// GET .../runs/{run_id}, ...).
//
// A run is asynchronous: it starts `queued` and the client polls it (or uses
// Client::pollRun()) until isTerminal(). The one state that is neither transient
// nor terminal is `requires_action` — the model called a tool the client owns,
// and the run stays parked until the outputs are submitted:
//
//     if (run.requiresAction()) {
//         QList<Core::ToolOutput> outputs;
//         for (const Core::ToolCall &call : run.requiredToolCalls())
//             outputs.append({call.id(), registry.invoke(call).content()});
//         client.submitToolOutputs(run.threadId(), run.id(), outputs);
//     }
//
// The tool calls come back as the same ToolCall type the Chat Completions path
// produces, so a ToolRegistry can dispatch them unchanged. `tools`,
// `truncation_strategy`, `tool_choice` and `response_format` are open unions and
// are carried verbatim.
class QTOPENAI_CORE_EXPORT Run
{
public:
    Run();
    Run(const Run &other);
    Run(Run &&other) noexcept;
    Run &operator=(const Run &other);
    Run &operator=(Run &&other) noexcept;
    ~Run();

    void swap(Run &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "thread.run".
    QString object() const;
    void setObject(const QString &object);

    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    QString threadId() const;
    void setThreadId(const QString &threadId);

    QString assistantId() const;
    void setAssistantId(const QString &assistantId);

    RunStatus status() const;
    void setStatus(RunStatus status);

    // The `required_action` type, normally "submit_tool_outputs"; empty when the
    // run is not waiting on the client.
    QString requiredActionType() const;
    void setRequiredActionType(const QString &requiredActionType);

    // The tool calls a `requires_action` run is waiting for
    // (`required_action.submit_tool_outputs.tool_calls`).
    QList<ToolCall> requiredToolCalls() const;
    void setRequiredToolCalls(const QList<ToolCall> &toolCalls);

    // The failure code/message from `last_error`; both empty when the run had no
    // error.
    QString errorCode() const;
    void setErrorCode(const QString &errorCode);

    QString errorMessage() const;
    void setErrorMessage(const QString &errorMessage);

    // Why a run ended `incomplete` (`incomplete_details`), verbatim.
    QJsonObject incompleteDetails() const;
    void setIncompleteDetails(const QJsonObject &incompleteDetails);

    // Lifecycle timestamps; 0 when the run has not reached that point.
    qint64 expiresAt() const;
    void setExpiresAt(qint64 expiresAt);

    qint64 startedAt() const;
    void setStartedAt(qint64 startedAt);

    qint64 cancelledAt() const;
    void setCancelledAt(qint64 cancelledAt);

    qint64 failedAt() const;
    void setFailedAt(qint64 failedAt);

    qint64 completedAt() const;
    void setCompletedAt(qint64 completedAt);

    // What the run was executed with — the assistant's settings, or the
    // overrides the create call passed.
    QString model() const;
    void setModel(const QString &model);

    QString instructions() const;
    void setInstructions(const QString &instructions);

    QJsonArray tools() const;
    void setTools(const QJsonArray &tools);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    // Token accounting; all zero until the run finishes.
    Usage usage() const;
    void setUsage(const Usage &usage);

    std::optional<double> temperature() const;
    void setTemperature(double temperature);

    std::optional<double> topP() const;
    void setTopP(double topP);

    std::optional<int> maxPromptTokens() const;
    void setMaxPromptTokens(int maxPromptTokens);

    std::optional<int> maxCompletionTokens() const;
    void setMaxCompletionTokens(int maxCompletionTokens);

    // How the thread was trimmed to fit the context window
    // (`truncation_strategy`), verbatim.
    QJsonObject truncationStrategy() const;
    void setTruncationStrategy(const QJsonObject &truncationStrategy);

    // tool_choice: "none"/"auto"/"required" or a specific tool object.
    QJsonValue toolChoice() const;
    void setToolChoice(const QJsonValue &toolChoice);

    std::optional<bool> parallelToolCalls() const;
    void setParallelToolCalls(bool parallelToolCalls);

    // response_format: "auto" or a format object.
    QJsonValue responseFormat() const;
    void setResponseFormat(const QJsonValue &responseFormat);

    // True once the run has reached a state it will no longer leave (Completed,
    // Failed, Cancelled, Incomplete or Expired); polling can stop.
    bool isTerminal() const;

    // True while the run is parked waiting for submitToolOutputs(). Not
    // terminal: the run continues once the outputs arrive.
    bool requiresAction() const;

    QJsonObject toJson() const;
    static Run fromJson(const QJsonObject &json);

    bool operator==(const Run &other) const;
    bool operator!=(const Run &other) const { return !(*this == other); }

private:
    QSharedDataPointer<RunData> d;
};

// A `list` of runs (GET /threads/{id}/runs).
using RunList = ListPage<Run>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Run)
Q_DECLARE_METATYPE(QtOpenAi::Core::Run)
Q_DECLARE_METATYPE(QtOpenAi::Core::RunList)
Q_DECLARE_METATYPE(QtOpenAi::Core::ToolOutput)
