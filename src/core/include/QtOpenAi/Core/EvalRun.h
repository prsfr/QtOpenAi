// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/Enums.h>
#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

// How an eval run's items scored. A plain value aggregate like
// BatchRequestCounts: four counters with no growth path.
struct QTOPENAI_CORE_EXPORT EvalResultCounts
{
    int total = 0;
    int errored = 0;
    int failed = 0;
    int passed = 0;

    QJsonObject toJson() const;
    static EvalResultCounts fromJson(const QJsonObject &json);

    bool operator==(const EvalResultCounts &other) const
    {
        return total == other.total && errored == other.errored && failed == other.failed
               && passed == other.passed;
    }
    bool operator!=(const EvalResultCounts &other) const { return !(*this == other); }
};

class EvalRunData;

// One execution of an eval against a data source
// (POST /evals/{eval_id}/runs, GET .../runs/{run_id}, ...).
//
// A run starts `queued` and scores its items asynchronously, so the client polls
// GET .../runs/{run_id} (or uses Client::pollEvalRun()) until it is terminal.
// `data_source` is an open union in the spec and is carried as raw JSON, like
// the eval's own config.
//
// The deletion acknowledgement of DELETE .../runs/{run_id} also decodes into
// this type; it names the id `run_id`, which fromJson() accepts as an
// alternative spelling of `id`.
class QTOPENAI_CORE_EXPORT EvalRun
{
public:
    EvalRun();
    EvalRun(const EvalRun &other);
    EvalRun(EvalRun &&other) noexcept;
    EvalRun &operator=(const EvalRun &other);
    EvalRun &operator=(EvalRun &&other) noexcept;
    ~EvalRun();

    void swap(EvalRun &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "eval.run" (or "eval.run.deleted").
    QString object() const;
    void setObject(const QString &object);

    // The eval this run belongs to.
    QString evalId() const;
    void setEvalId(const QString &evalId);

    QString name() const;
    void setName(const QString &name);

    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    EvalRunStatus status() const;
    void setStatus(EvalRunStatus status);

    // The model under test, when the data source names one.
    QString model() const;
    void setModel(const QString &model);

    // Link to the rendered report in the OpenAI dashboard.
    QString reportUrl() const;
    void setReportUrl(const QString &reportUrl);

    EvalResultCounts resultCounts() const;
    void setResultCounts(const EvalResultCounts &resultCounts);

    // What the run was executed against (`data_source`), verbatim.
    QJsonObject dataSource() const;
    void setDataSource(const QJsonObject &dataSource);

    // The failure code/message from the `error` object; both empty when the run
    // had no run-level error.
    QString errorCode() const;
    void setErrorCode(const QString &errorCode);

    QString errorMessage() const;
    void setErrorMessage(const QString &errorMessage);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    // True once the run has reached a state it will no longer leave (Completed,
    // Failed or Canceled); polling can stop.
    bool isTerminal() const;

    QJsonObject toJson() const;
    static EvalRun fromJson(const QJsonObject &json);

    bool operator==(const EvalRun &other) const;
    bool operator!=(const EvalRun &other) const { return !(*this == other); }

private:
    QSharedDataPointer<EvalRunData> d;
};

// A `list` of eval runs (GET /evals/{eval_id}/runs).
using EvalRunList = ListPage<EvalRun>;

class EvalRunOutputItemData;

// The outcome for a single item of a run
// (GET /evals/{eval_id}/runs/{run_id}/output_items). It pairs the input item
// with the model's sample and each grader's result.
class QTOPENAI_CORE_EXPORT EvalRunOutputItem
{
public:
    EvalRunOutputItem();
    EvalRunOutputItem(const EvalRunOutputItem &other);
    EvalRunOutputItem(EvalRunOutputItem &&other) noexcept;
    EvalRunOutputItem &operator=(const EvalRunOutputItem &other);
    EvalRunOutputItem &operator=(EvalRunOutputItem &&other) noexcept;
    ~EvalRunOutputItem();

    void swap(EvalRunOutputItem &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "eval.run.output_item".
    QString object() const;
    void setObject(const QString &object);

    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    QString runId() const;
    void setRunId(const QString &runId);

    QString evalId() const;
    void setEvalId(const QString &evalId);

    // Per-item outcome, e.g. "pass" or "fail". Kept as a string: it is a grader
    // verdict, unrelated to the run's own status set.
    QString status() const;
    void setStatus(const QString &status);

    // Index of the source item within the data source.
    int datasourceItemId() const;
    void setDatasourceItemId(int datasourceItemId);

    // The input item, the graders' results and the model's sample, verbatim —
    // all three are shaped by the eval's own configuration.
    QJsonObject datasourceItem() const;
    void setDatasourceItem(const QJsonObject &datasourceItem);

    QJsonArray results() const;
    void setResults(const QJsonArray &results);

    QJsonObject sample() const;
    void setSample(const QJsonObject &sample);

    QJsonObject toJson() const;
    static EvalRunOutputItem fromJson(const QJsonObject &json);

    bool operator==(const EvalRunOutputItem &other) const;
    bool operator!=(const EvalRunOutputItem &other) const { return !(*this == other); }

private:
    QSharedDataPointer<EvalRunOutputItemData> d;
};

// A `list` of run output items.
using EvalRunOutputItemList = ListPage<EvalRunOutputItem>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::EvalRun)
Q_DECLARE_SHARED(QtOpenAi::Core::EvalRunOutputItem)
Q_DECLARE_METATYPE(QtOpenAi::Core::EvalRun)
Q_DECLARE_METATYPE(QtOpenAi::Core::EvalRunList)
Q_DECLARE_METATYPE(QtOpenAi::Core::EvalRunOutputItem)
Q_DECLARE_METATYPE(QtOpenAi::Core::EvalRunOutputItemList)
