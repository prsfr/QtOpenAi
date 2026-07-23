// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class ModerationResultData;

// One moderation result: whether the input was flagged, plus the per-category
// booleans, scores, and (optionally) which input types each category applied to.
// Categories are provider-defined, so they are kept as string-keyed maps.
class QTOPENAI_CORE_EXPORT ModerationResult
{
public:
    ModerationResult();
    ModerationResult(const ModerationResult &other);
    ModerationResult(ModerationResult &&other) noexcept;
    ModerationResult &operator=(const ModerationResult &other);
    ModerationResult &operator=(ModerationResult &&other) noexcept;
    ~ModerationResult();

    void swap(ModerationResult &other) noexcept { d.swap(other.d); }

    bool flagged() const;
    void setFlagged(bool flagged);

    QMap<QString, bool> categories() const;
    void setCategories(const QMap<QString, bool> &categories);

    QMap<QString, double> categoryScores() const;
    void setCategoryScores(const QMap<QString, double> &scores);

    // Which input types (e.g. "text", "image") each category applied to.
    QJsonObject categoryAppliedInputTypes() const;
    void setCategoryAppliedInputTypes(const QJsonObject &applied);

    // Convenience: is a specific category flagged / its score (0 if absent).
    bool isFlagged(const QString &category) const;
    double score(const QString &category) const;

    QJsonObject toJson() const;
    static ModerationResult fromJson(const QJsonObject &json);

    bool operator==(const ModerationResult &other) const;
    bool operator!=(const ModerationResult &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ModerationResultData> d;
};

class ModerationResponseData;

// A parsed `moderation` response (POST /moderations): an id, the model used, and
// one result per input.
class QTOPENAI_CORE_EXPORT ModerationResponse
{
public:
    ModerationResponse();
    ModerationResponse(const ModerationResponse &other);
    ModerationResponse(ModerationResponse &&other) noexcept;
    ModerationResponse &operator=(const ModerationResponse &other);
    ModerationResponse &operator=(ModerationResponse &&other) noexcept;
    ~ModerationResponse();

    void swap(ModerationResponse &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    QString model() const;
    void setModel(const QString &model);

    QList<ModerationResult> results() const;
    void setResults(const QList<ModerationResult> &results);

    // Convenience: the first result, or a default-constructed one.
    ModerationResult firstResult() const;

    QJsonObject toJson() const;
    static ModerationResponse fromJson(const QJsonObject &json);

    bool operator==(const ModerationResponse &other) const;
    bool operator!=(const ModerationResponse &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ModerationResponseData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ModerationResult)
Q_DECLARE_SHARED(QtOpenAi::Core::ModerationResponse)
