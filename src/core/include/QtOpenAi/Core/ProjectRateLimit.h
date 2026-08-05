// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {

class ProjectRateLimitData;

// One model's rate limit within a project
// (GET /organization/projects/{id}/rate_limits, POST .../{rate_limit_id}).
//
// **Every limit is optional, and that is the whole point.** The same type is
// both what the server reports and what an update sends, because an update is a
// partial one: only the limits the caller actually set go on the wire. A plain
// `int` would have made "leave this alone" and "set this to zero" the same
// value, and zero here means the model is unusable in that project.
//
//     Core::ProjectRateLimit limits;
//     limits.setMaxRequestsPerMinute(600);          // the only field sent
//     organization.modifyProjectRateLimit(projectId, rateLimitId, limits);
//
// Reading one back fills in whichever limits the server reports; the rest stay
// unset rather than becoming zeros, so a round trip does not invent a limit the
// server never mentioned.
class QTOPENAI_CORE_EXPORT ProjectRateLimit
{
public:
    ProjectRateLimit();
    ProjectRateLimit(const ProjectRateLimit &other);
    ProjectRateLimit(ProjectRateLimit &&other) noexcept;
    ProjectRateLimit &operator=(const ProjectRateLimit &other);
    ProjectRateLimit &operator=(ProjectRateLimit &&other) noexcept;
    ~ProjectRateLimit();

    void swap(ProjectRateLimit &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // Normally "project.rate_limit".
    QString object() const;
    void setObject(const QString &object);

    // The model the limit applies to, e.g. "gpt-4o".
    QString model() const;
    void setModel(const QString &model);

    // The limits. Each is unset until the server reports it or the caller sets
    // it; see the class note for why that is not an `int` with a sentinel.
    std::optional<qint64> maxRequestsPerMinute() const;
    void setMaxRequestsPerMinute(std::optional<qint64> value);

    std::optional<qint64> maxTokensPerMinute() const;
    void setMaxTokensPerMinute(std::optional<qint64> value);

    std::optional<qint64> maxImagesPerMinute() const;
    void setMaxImagesPerMinute(std::optional<qint64> value);

    std::optional<qint64> maxAudioMegabytesPerMinute() const;
    void setMaxAudioMegabytesPerMinute(std::optional<qint64> value);

    std::optional<qint64> maxRequestsPerDay() const;
    void setMaxRequestsPerDay(std::optional<qint64> value);

    std::optional<qint64> batchMaxInputTokensPerDay() const;
    void setBatchMaxInputTokensPerDay(std::optional<qint64> value);

    // True when no limit is set at all, i.e. an update that would change
    // nothing. Worth checking before sending one.
    bool isEmpty() const;

    QJsonObject toJson() const;
    static ProjectRateLimit fromJson(const QJsonObject &json);

    bool operator==(const ProjectRateLimit &other) const;
    bool operator!=(const ProjectRateLimit &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ProjectRateLimitData> d;
};

using ProjectRateLimitList = ListPage<ProjectRateLimit>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ProjectRateLimit)
Q_DECLARE_METATYPE(QtOpenAi::Core::ProjectRateLimit)
Q_DECLARE_METATYPE(QtOpenAi::Core::ProjectRateLimitList)
