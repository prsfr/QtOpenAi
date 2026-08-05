// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/BucketPage.h>
#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMap>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {

class UsageResultData;

// One row of an organization usage report (GET /organization/usage/*).
//
// Named for the endpoint family rather than after the file: Core/Usage.h is the
// token count of a single completion, which is a different thing entirely.
//
// **One result type for all ten endpoints, not ten.** The endpoints differ only
// in which counters a row carries — `input_tokens` for completions, `characters`
// for speech, `usage_bytes` for vector stores — while the grouping keys and the
// bucket around them are identical. Ten classes differing by one integer each
// would be ten places to keep in step, and any counter this library guessed
// wrong would decode as a silent zero. So the grouping keys are typed members
// and the counters live in a map: the ones OpenAI documents have named accessors
// below, and one the API grows tomorrow still survives a round trip instead of
// being dropped on the floor.
class QTOPENAI_CORE_EXPORT UsageResult
{
public:
    UsageResult();
    UsageResult(const UsageResult &other);
    UsageResult(UsageResult &&other) noexcept;
    UsageResult &operator=(const UsageResult &other);
    UsageResult &operator=(UsageResult &&other) noexcept;
    ~UsageResult();

    void swap(UsageResult &other) noexcept { d.swap(other.d); }

    // The object type, e.g. "organization.usage.completions.result". The one
    // field that says which endpoint this row came from.
    QString object() const;
    void setObject(const QString &object);

    // --- Grouping keys -----------------------------------------------------
    // Set only when the query asked to group by them; the API sends null
    // otherwise, which reads back as an empty string here.
    QString projectId() const;
    void setProjectId(const QString &projectId);

    QString userId() const;
    void setUserId(const QString &userId);

    QString apiKeyId() const;
    void setApiKeyId(const QString &apiKeyId);

    QString model() const;
    void setModel(const QString &model);

    // Images only: "image.generation", "image.edit", … and the image size.
    QString source() const;
    void setSource(const QString &source);

    QString size() const;
    void setSize(const QString &size);

    // Completions only, and genuinely tri-state: true for batch usage, false for
    // interactive usage, unset when the query did not group by it. A plain bool
    // would report ungrouped totals as interactive.
    std::optional<bool> batch() const;
    void setBatch(std::optional<bool> batch);

    // --- Counters ----------------------------------------------------------
    // The counter under its wire name, or 0 when this row has none — an absent
    // counter and a zero one mean the same thing in a usage report.
    qint64 metric(const QString &name) const;
    void setMetric(const QString &name, qint64 value);

    // Every counter the row carries, wire name to value. Sorted, so a report
    // serialises the same way twice.
    QMap<QString, qint64> metrics() const;
    void setMetrics(const QMap<QString, qint64> &metrics);

    // The counters OpenAI documents, by the endpoint that reports them. Each is
    // the corresponding metric() lookup, spelled out so the common path is typed
    // and discoverable rather than a string a caller has to know.
    qint64 inputTokens() const { return metric(QStringLiteral("input_tokens")); }
    qint64 outputTokens() const { return metric(QStringLiteral("output_tokens")); }
    qint64 inputCachedTokens() const { return metric(QStringLiteral("input_cached_tokens")); }
    qint64 inputAudioTokens() const { return metric(QStringLiteral("input_audio_tokens")); }
    qint64 outputAudioTokens() const { return metric(QStringLiteral("output_audio_tokens")); }
    qint64 numModelRequests() const { return metric(QStringLiteral("num_model_requests")); }
    qint64 images() const { return metric(QStringLiteral("images")); }
    qint64 characters() const { return metric(QStringLiteral("characters")); }
    qint64 seconds() const { return metric(QStringLiteral("seconds")); }
    qint64 usageBytes() const { return metric(QStringLiteral("usage_bytes")); }
    qint64 numSessions() const { return metric(QStringLiteral("num_sessions")); }
    qint64 numCalls() const { return metric(QStringLiteral("num_calls")); }

    // Convenience for the completions endpoint, where a caller almost always
    // wants the two token counts together.
    qint64 totalTokens() const { return inputTokens() + outputTokens(); }

    QJsonObject toJson() const;
    static UsageResult fromJson(const QJsonObject &json);

    bool operator==(const UsageResult &other) const;
    bool operator!=(const UsageResult &other) const { return !(*this == other); }

private:
    QSharedDataPointer<UsageResultData> d;
};

// One time bucket of a usage report, and the page of them an endpoint returns
// (the OpenAPI `UsageResponse`).
using UsageBucket = Bucket<UsageResult>;
using UsagePage = BucketPage<UsageResult>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::UsageResult)
Q_DECLARE_METATYPE(QtOpenAi::Core::UsageResult)
Q_DECLARE_METATYPE(QtOpenAi::Core::UsageBucket)
Q_DECLARE_METATYPE(QtOpenAi::Core::UsagePage)
