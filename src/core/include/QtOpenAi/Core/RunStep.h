// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/Enums.h>
#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>
#include <QtOpenAi/Core/Usage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class RunStepData;

// One step a run took (GET /threads/{id}/runs/{run_id}/steps, .../steps/{id}).
//
// Steps are the audit trail of a run: either the assistant wrote a message
// (`message_creation`) or it invoked tools (`tool_calls`). Which of the two a
// step is says what `step_details` contains, and that object is an open union
// spanning code-interpreter output, file-search results and function calls, so
// it is carried verbatim.
class QTOPENAI_CORE_EXPORT RunStep
{
public:
    RunStep();
    RunStep(const RunStep &other);
    RunStep(RunStep &&other) noexcept;
    RunStep &operator=(const RunStep &other);
    RunStep &operator=(RunStep &&other) noexcept;
    ~RunStep();

    void swap(RunStep &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "thread.run.step".
    QString object() const;
    void setObject(const QString &object);

    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    QString assistantId() const;
    void setAssistantId(const QString &assistantId);

    QString threadId() const;
    void setThreadId(const QString &threadId);

    QString runId() const;
    void setRunId(const QString &runId);

    // "message_creation" or "tool_calls".
    QString type() const;
    void setType(const QString &type);

    RunStepStatus status() const;
    void setStatus(RunStepStatus status);

    // What the step did (`step_details`), verbatim.
    QJsonObject stepDetails() const;
    void setStepDetails(const QJsonObject &stepDetails);

    // The failure code/message from `last_error`; both empty when the step had
    // no error.
    QString errorCode() const;
    void setErrorCode(const QString &errorCode);

    QString errorMessage() const;
    void setErrorMessage(const QString &errorMessage);

    qint64 expiredAt() const;
    void setExpiredAt(qint64 expiredAt);

    qint64 cancelledAt() const;
    void setCancelledAt(qint64 cancelledAt);

    qint64 failedAt() const;
    void setFailedAt(qint64 failedAt);

    qint64 completedAt() const;
    void setCompletedAt(qint64 completedAt);

    Usage usage() const;
    void setUsage(const Usage &usage);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    // True once the step has reached a state it will no longer leave.
    bool isTerminal() const;

    QJsonObject toJson() const;
    static RunStep fromJson(const QJsonObject &json);

    bool operator==(const RunStep &other) const;
    bool operator!=(const RunStep &other) const { return !(*this == other); }

private:
    QSharedDataPointer<RunStepData> d;
};

// A `list` of run steps (GET /threads/{id}/runs/{run_id}/steps).
using RunStepList = ListPage<RunStep>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::RunStep)
Q_DECLARE_METATYPE(QtOpenAi::Core::RunStep)
Q_DECLARE_METATYPE(QtOpenAi::Core::RunStepList)
