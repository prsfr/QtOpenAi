// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/Enums.h>
#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

// How many of a batch's requests have been processed so far. A lightweight value
// aggregate like VectorStoreFileCounts rather than a d-pointer type: three
// counters with no growth path.
struct QTOPENAI_CORE_EXPORT BatchRequestCounts
{
    int total = 0;
    int completed = 0;
    int failed = 0;

    QJsonObject toJson() const;
    static BatchRequestCounts fromJson(const QJsonObject &json);

    bool operator==(const BatchRequestCounts &other) const
    {
        return total == other.total && completed == other.completed && failed == other.failed;
    }
    bool operator!=(const BatchRequestCounts &other) const { return !(*this == other); }
};

// One entry of a batch's `errors` list — a problem with the input file that
// stopped the batch from being accepted, pointing at the offending JSONL line.
struct QTOPENAI_CORE_EXPORT BatchError
{
    QString code;
    QString message;
    // The input-file field the error refers to; empty when it applies to the
    // whole line.
    QString param;
    // 1-based line number in the input file; 0 when the API did not report one.
    int line = 0;

    QJsonObject toJson() const;
    static BatchError fromJson(const QJsonObject &json);

    bool operator==(const BatchError &other) const
    {
        return code == other.code && message == other.message && param == other.param
               && line == other.line;
    }
    bool operator!=(const BatchError &other) const { return !(*this == other); }
};

class BatchData;

// One asynchronous batch job (POST /batches, GET /batches/{id}, ...).
//
// A batch runs a whole JSONL file of requests against a single endpoint at a
// discount, within a completion window. Creating one returns it in the
// `validating` state; the client polls GET /batches/{id} (or uses
// Client::pollBatch()) until it becomes terminal, then downloads the results
// from the Files API using outputFileId() — and errorFileId() for the requests
// that failed.
class QTOPENAI_CORE_EXPORT Batch
{
public:
    Batch();
    Batch(const Batch &other);
    Batch(Batch &&other) noexcept;
    Batch &operator=(const Batch &other);
    Batch &operator=(Batch &&other) noexcept;
    ~Batch();

    void swap(Batch &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "batch".
    QString object() const;
    void setObject(const QString &object);

    // The API route every request in the input file targets, e.g.
    // "/v1/chat/completions".
    QString endpoint() const;
    void setEndpoint(const QString &endpoint);

    // Id of the uploaded JSONL input file (Files API, purpose "batch").
    QString inputFileId() const;
    void setInputFileId(const QString &inputFileId);

    // The time frame the batch must finish in, e.g. "24h".
    QString completionWindow() const;
    void setCompletionWindow(const QString &completionWindow);

    BatchStatus status() const;
    void setStatus(BatchStatus status);

    // Id of the JSONL file holding the successful responses; empty until the
    // batch completes.
    QString outputFileId() const;
    void setOutputFileId(const QString &outputFileId);

    // Id of the JSONL file holding the failed requests; empty when none failed.
    QString errorFileId() const;
    void setErrorFileId(const QString &errorFileId);

    // Lifecycle timestamps (Unix seconds). Each is 0 while the corresponding
    // transition has not happened — the API omits them rather than sending 0.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    qint64 inProgressAt() const;
    void setInProgressAt(qint64 inProgressAt);

    qint64 expiresAt() const;
    void setExpiresAt(qint64 expiresAt);

    qint64 finalizingAt() const;
    void setFinalizingAt(qint64 finalizingAt);

    qint64 completedAt() const;
    void setCompletedAt(qint64 completedAt);

    qint64 failedAt() const;
    void setFailedAt(qint64 failedAt);

    qint64 expiredAt() const;
    void setExpiredAt(qint64 expiredAt);

    qint64 cancellingAt() const;
    void setCancellingAt(qint64 cancellingAt);

    qint64 cancelledAt() const;
    void setCancelledAt(qint64 cancelledAt);

    BatchRequestCounts requestCounts() const;
    void setRequestCounts(const BatchRequestCounts &requestCounts);

    // Input-validation problems that stopped the batch; empty in the normal case.
    QList<BatchError> errors() const;
    void setErrors(const QList<BatchError> &errors);

    // Caller-supplied key/value pairs echoed back by the API.
    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    // True once the batch has reached a state it will no longer leave
    // (Completed, Failed, Expired or Cancelled); polling can stop. Note that
    // Cancelling is *not* terminal — the batch still settles on Cancelled.
    bool isTerminal() const;

    QJsonObject toJson() const;
    static Batch fromJson(const QJsonObject &json);

    bool operator==(const Batch &other) const;
    bool operator!=(const Batch &other) const { return !(*this == other); }

private:
    QSharedDataPointer<BatchData> d;
};

// A `list` of batches (GET /batches). Cursor-paginated; reuses the shared
// list-page type.
using BatchList = ListPage<Batch>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Batch)
Q_DECLARE_METATYPE(QtOpenAi::Core::Batch)
Q_DECLARE_METATYPE(QtOpenAi::Core::BatchList)
