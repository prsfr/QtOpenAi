// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/CreateThreadRequest.h>
#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ResponseFormat.h>
#include <QtOpenAi/Core/Tool.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {

class CreateRunRequestData;

// The body of a POST /threads/{id}/runs request: which assistant to run, plus
// the per-run overrides of whatever the assistant was configured with.
//
// The same body, with setThread() filled in, creates a thread and runs it in one
// call (POST /threads/runs).
class QTOPENAI_CORE_EXPORT CreateRunRequest
{
public:
    CreateRunRequest();
    explicit CreateRunRequest(QString assistantId);
    CreateRunRequest(const CreateRunRequest &other);
    CreateRunRequest(CreateRunRequest &&other) noexcept;
    CreateRunRequest &operator=(const CreateRunRequest &other);
    CreateRunRequest &operator=(CreateRunRequest &&other) noexcept;
    ~CreateRunRequest();

    void swap(CreateRunRequest &other) noexcept { d.swap(other.d); }

    QString assistantId() const;
    void setAssistantId(const QString &assistantId);

    // Override the assistant's model for this run.
    QString model() const;
    void setModel(const QString &model);

    // Replace the assistant's instructions for this run.
    QString instructions() const;
    void setInstructions(const QString &instructions);

    // Append to them instead of replacing them.
    QString additionalInstructions() const;
    void setAdditionalInstructions(const QString &additionalInstructions);

    // Messages appended to the thread before the run starts.
    QList<ThreadMessageInput> additionalMessages() const;
    void setAdditionalMessages(const QList<ThreadMessageInput> &messages);
    void addMessage(const ThreadMessageInput &message);
    void addUserMessage(const QString &text);

    // Override the assistant's tools for this run. Open union, like the
    // assistant's own — addTool() takes a typed function tool or raw JSON.
    QJsonArray tools() const;
    void setTools(const QJsonArray &tools);
    void addTool(const Tool &tool);
    void addTool(const QJsonObject &tool);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    std::optional<double> temperature() const;
    void setTemperature(double temperature);

    std::optional<double> topP() const;
    void setTopP(double topP);

    // Context-window budgets; the run ends `incomplete` when it would exceed
    // one.
    std::optional<int> maxPromptTokens() const;
    void setMaxPromptTokens(int maxPromptTokens);

    std::optional<int> maxCompletionTokens() const;
    void setMaxCompletionTokens(int maxCompletionTokens);

    // How to trim the thread to fit (`truncation_strategy`), verbatim.
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
    void setResponseFormat(const ResponseFormat &responseFormat);

    // The thread to create alongside the run (POST /threads/runs only).
    CreateThreadRequest thread() const;
    void setThread(const CreateThreadRequest &thread);

    // Server-Sent-Events streaming. Client::createRunStream() sets this for you;
    // it is exposed because the streaming request path is shared with the other
    // streamed endpoints.
    std::optional<bool> stream() const;
    void setStream(bool stream);

    QJsonObject toJson() const;

    bool operator==(const CreateRunRequest &other) const;
    bool operator!=(const CreateRunRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CreateRunRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::CreateRunRequest)
