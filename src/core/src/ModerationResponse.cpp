// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ModerationResponse.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ModerationResultData : public QSharedData
{
public:
    bool flagged = false;
    QMap<QString, bool> categories;
    QMap<QString, double> categoryScores;
    QJsonObject categoryAppliedInputTypes;
};

ModerationResult::ModerationResult()
    : d(new ModerationResultData)
{ }

ModerationResult::ModerationResult(const ModerationResult &other) = default;
ModerationResult::ModerationResult(ModerationResult &&other) noexcept = default;
ModerationResult &ModerationResult::operator=(const ModerationResult &other) = default;
ModerationResult &ModerationResult::operator=(ModerationResult &&other) noexcept = default;
ModerationResult::~ModerationResult() = default;

bool ModerationResult::flagged() const { return d->flagged; }
void ModerationResult::setFlagged(bool flagged) { d->flagged = flagged; }

QMap<QString, bool> ModerationResult::categories() const { return d->categories; }
void ModerationResult::setCategories(const QMap<QString, bool> &categories)
{
    d->categories = categories;
}

QMap<QString, double> ModerationResult::categoryScores() const { return d->categoryScores; }
void ModerationResult::setCategoryScores(const QMap<QString, double> &scores)
{
    d->categoryScores = scores;
}

QJsonObject ModerationResult::categoryAppliedInputTypes() const
{
    return d->categoryAppliedInputTypes;
}
void ModerationResult::setCategoryAppliedInputTypes(const QJsonObject &applied)
{
    d->categoryAppliedInputTypes = applied;
}

bool ModerationResult::isFlagged(const QString &category) const
{
    return d->categories.value(category, false);
}

double ModerationResult::score(const QString &category) const
{
    return d->categoryScores.value(category, 0.0);
}

QJsonObject ModerationResult::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("flagged"), d->flagged);

    QJsonObject categories;
    for (auto it = d->categories.constBegin(); it != d->categories.constEnd(); ++it)
        categories.insert(it.key(), it.value());
    json.insert(QStringLiteral("categories"), categories);

    QJsonObject scores;
    for (auto it = d->categoryScores.constBegin(); it != d->categoryScores.constEnd(); ++it)
        scores.insert(it.key(), it.value());
    json.insert(QStringLiteral("category_scores"), scores);

    if (!d->categoryAppliedInputTypes.isEmpty())
        json.insert(QStringLiteral("category_applied_input_types"), d->categoryAppliedInputTypes);
    return json;
}

ModerationResult ModerationResult::fromJson(const QJsonObject &json)
{
    ModerationResult result;
    result.d->flagged = json.value(QStringLiteral("flagged")).toBool();

    const QJsonObject categories = json.value(QStringLiteral("categories")).toObject();
    for (auto it = categories.constBegin(); it != categories.constEnd(); ++it)
        result.d->categories.insert(it.key(), it.value().toBool());

    const QJsonObject scores = json.value(QStringLiteral("category_scores")).toObject();
    for (auto it = scores.constBegin(); it != scores.constEnd(); ++it)
        result.d->categoryScores.insert(it.key(), it.value().toDouble());

    result.d->categoryAppliedInputTypes
            = json.value(QStringLiteral("category_applied_input_types")).toObject();
    return result;
}

bool ModerationResult::operator==(const ModerationResult &other) const
{
    return d->flagged == other.d->flagged && d->categories == other.d->categories
           && d->categoryScores == other.d->categoryScores
           && d->categoryAppliedInputTypes == other.d->categoryAppliedInputTypes;
}

class ModerationResponseData : public QSharedData
{
public:
    QString id;
    QString model;
    QList<ModerationResult> results;
};

ModerationResponse::ModerationResponse()
    : d(new ModerationResponseData)
{ }

ModerationResponse::ModerationResponse(const ModerationResponse &other) = default;
ModerationResponse::ModerationResponse(ModerationResponse &&other) noexcept = default;
ModerationResponse &ModerationResponse::operator=(const ModerationResponse &other) = default;
ModerationResponse &ModerationResponse::operator=(ModerationResponse &&other) noexcept = default;
ModerationResponse::~ModerationResponse() = default;

QString ModerationResponse::id() const { return d->id; }
void ModerationResponse::setId(const QString &id) { d->id = id; }

QString ModerationResponse::model() const { return d->model; }
void ModerationResponse::setModel(const QString &model) { d->model = model; }

QList<ModerationResult> ModerationResponse::results() const { return d->results; }
void ModerationResponse::setResults(const QList<ModerationResult> &results)
{
    d->results = results;
}

ModerationResult ModerationResponse::firstResult() const
{
    return d->results.isEmpty() ? ModerationResult() : d->results.first();
}

QJsonObject ModerationResponse::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("id"), d->id);
    json.insert(QStringLiteral("model"), d->model);
    QJsonArray results;
    for (const ModerationResult &result : d->results)
        results.append(result.toJson());
    json.insert(QStringLiteral("results"), results);
    return json;
}

ModerationResponse ModerationResponse::fromJson(const QJsonObject &json)
{
    ModerationResponse response;
    response.d->id = detail::stringOr(json, QStringLiteral("id"));
    response.d->model = detail::stringOr(json, QStringLiteral("model"));
    const QJsonArray results = json.value(QStringLiteral("results")).toArray();
    for (const QJsonValue &value : results)
        response.d->results.append(ModerationResult::fromJson(value.toObject()));
    return response;
}

bool ModerationResponse::operator==(const ModerationResponse &other) const
{
    return d->id == other.d->id && d->model == other.d->model && d->results == other.d->results;
}

} // namespace Core
} // namespace QtOpenAi
