// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/VectorIndex.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

#include <algorithm>
#include <cmath>

namespace QtOpenAi {
namespace Core {

namespace Vector {

double dot(const QList<double> &a, const QList<double> &b)
{
    if (a.size() != b.size())
        return 0.0;
    double total = 0.0;
    for (qsizetype i = 0; i < a.size(); ++i)
        total += a.at(i) * b.at(i);
    return total;
}

double norm(const QList<double> &vector) { return std::sqrt(dot(vector, vector)); }

double cosineSimilarity(const QList<double> &a, const QList<double> &b)
{
    const double denominator = norm(a) * norm(b);
    // A zero vector has no direction, so it is not similar to anything -- an
    // honest 0 rather than a division by zero.
    return denominator > 0.0 ? dot(a, b) / denominator : 0.0;
}

double euclideanDistance(const QList<double> &a, const QList<double> &b)
{
    if (a.size() != b.size())
        return 0.0;
    double total = 0.0;
    for (qsizetype i = 0; i < a.size(); ++i) {
        const double difference = a.at(i) - b.at(i);
        total += difference * difference;
    }
    return std::sqrt(total);
}

QList<double> normalized(const QList<double> &vector)
{
    const double length = norm(vector);
    if (length <= 0.0)
        return vector;
    QList<double> result;
    result.reserve(vector.size());
    for (double value : vector)
        result.append(value / length);
    return result;
}

} // namespace Vector

namespace {

struct Entry
{
    QList<double> vector;
    QString text;
    QJsonObject payload;
};

} // namespace

class VectorIndexData : public QSharedData
{
public:
    // Insertion order is kept alongside the map so ids(), toJson() and ties in
    // search() are reproducible. A ranking that reshuffles equal scores between
    // runs is a ranking nobody can test.
    QHash<QString, Entry> entries;
    QStringList order;
    VectorIndex::Metric metric = VectorIndex::Metric::Cosine;
    int dimension = 0;
};

VectorIndex::VectorIndex()
    : d(new VectorIndexData)
{ }

VectorIndex::VectorIndex(const VectorIndex &other) = default;
VectorIndex::VectorIndex(VectorIndex &&other) noexcept = default;
VectorIndex &VectorIndex::operator=(const VectorIndex &other) = default;
VectorIndex &VectorIndex::operator=(VectorIndex &&other) noexcept = default;
VectorIndex::~VectorIndex() = default;

bool VectorIndex::add(const QString &id, const QList<double> &vector, const QString &text,
                      const QJsonObject &payload)
{
    if (id.isEmpty() || vector.isEmpty())
        return false;
    // Vectors from two different embedding models rank against each other as
    // convincing nonsense. Refusing is the only way a caller finds out.
    if (d->dimension > 0 && vector.size() != d->dimension)
        return false;

    if (!d->entries.contains(id))
        d->order.append(id);
    d->entries.insert(id, Entry {vector, text, payload});
    d->dimension = int(vector.size());
    return true;
}

bool VectorIndex::remove(const QString &id)
{
    if (d->entries.remove(id) == 0)
        return false;
    d->order.removeOne(id);
    // An emptied index takes any dimension again, which is what makes
    // clear()-then-reindex with a different model work.
    if (d->entries.isEmpty())
        d->dimension = 0;
    return true;
}

void VectorIndex::clear()
{
    d->entries.clear();
    d->order.clear();
    d->dimension = 0;
}

bool VectorIndex::contains(const QString &id) const { return d->entries.contains(id); }
int VectorIndex::size() const { return int(d->entries.size()); }
bool VectorIndex::isEmpty() const { return d->entries.isEmpty(); }
int VectorIndex::dimension() const { return d->dimension; }
QStringList VectorIndex::ids() const { return d->order; }

QList<double> VectorIndex::vector(const QString &id) const { return d->entries.value(id).vector; }
QString VectorIndex::text(const QString &id) const { return d->entries.value(id).text; }
QJsonObject VectorIndex::payload(const QString &id) const { return d->entries.value(id).payload; }

VectorIndex::Metric VectorIndex::metric() const { return d->metric; }
void VectorIndex::setMetric(Metric metric) { d->metric = metric; }

QList<VectorMatch> VectorIndex::search(const QList<double> &query, int k, double minScore) const
{
    QList<VectorMatch> matches;
    if (query.isEmpty() || k <= 0 || d->entries.isEmpty())
        return matches;

    matches.reserve(d->order.size());
    for (const QString &id : d->order) {
        const Entry &entry = d->entries.value(id);

        double score = 0.0;
        switch (d->metric) {
        case Metric::Cosine:
            score = Vector::cosineSimilarity(query, entry.vector);
            break;
        case Metric::DotProduct:
            score = Vector::dot(query, entry.vector);
            break;
        case Metric::Euclidean:
            // Negated, so "higher is better" holds for every metric and callers
            // never have to ask which one produced a score.
            score = -Vector::euclideanDistance(query, entry.vector);
            break;
        }

        if (score < minScore)
            continue;
        matches.append(VectorMatch {id, score, entry.text, entry.payload});
    }

    // Stable, so entries that score equally come back in insertion order rather
    // than in whatever order the sort happened to leave them.
    std::stable_sort(matches.begin(), matches.end(),
                     [](const VectorMatch &a, const VectorMatch &b) { return a.score > b.score; });
    if (matches.size() > k)
        matches.resize(k);
    return matches;
}

QJsonObject VectorIndex::toJson() const
{
    QJsonArray entries;
    for (const QString &id : d->order) {
        const Entry &entry = d->entries.value(id);
        QJsonArray vector;
        for (double value : entry.vector)
            vector.append(value);

        QJsonObject object {{QStringLiteral("id"), id}, {QStringLiteral("vector"), vector}};
        if (!entry.text.isEmpty())
            object.insert(QStringLiteral("text"), entry.text);
        if (!entry.payload.isEmpty())
            object.insert(QStringLiteral("payload"), entry.payload);
        entries.append(object);
    }

    QString metric = QStringLiteral("cosine");
    if (d->metric == Metric::DotProduct)
        metric = QStringLiteral("dot_product");
    else if (d->metric == Metric::Euclidean)
        metric = QStringLiteral("euclidean");

    return {{QStringLiteral("metric"), metric}, {QStringLiteral("entries"), entries}};
}

VectorIndex VectorIndex::fromJson(const QJsonObject &json)
{
    VectorIndex index;

    const QString metric = json.value(QStringLiteral("metric")).toString();
    if (metric == QLatin1String("dot_product"))
        index.setMetric(Metric::DotProduct);
    else if (metric == QLatin1String("euclidean"))
        index.setMetric(Metric::Euclidean);

    const QJsonArray entries = json.value(QStringLiteral("entries")).toArray();
    for (const QJsonValue &value : entries) {
        const QJsonObject object = value.toObject();
        QList<double> vector;
        const QJsonArray numbers = object.value(QStringLiteral("vector")).toArray();
        vector.reserve(numbers.size());
        for (const QJsonValue &number : numbers)
            vector.append(number.toDouble());

        index.add(object.value(QStringLiteral("id")).toString(), vector,
                  object.value(QStringLiteral("text")).toString(),
                  object.value(QStringLiteral("payload")).toObject());
    }
    return index;
}

bool VectorIndex::operator==(const VectorIndex &other) const
{
    if (d->metric != other.d->metric || d->order != other.d->order)
        return false;
    for (const QString &id : d->order) {
        const Entry &mine = d->entries.value(id);
        const Entry &theirs = other.d->entries.value(id);
        if (mine.vector != theirs.vector || mine.text != theirs.text
            || mine.payload != theirs.payload)
            return false;
    }
    return true;
}

} // namespace Core
} // namespace QtOpenAi
