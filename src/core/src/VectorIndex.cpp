// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/VectorIndex.h"

#include "WireTable_p.h"

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

    // Four independent accumulators rather than one. Floating-point addition is
    // not associative, so a single running total is a dependency chain: the
    // compiler vectorised the multiply (mulpd in the shipped library) and had to
    // keep the sum serial (addsd), one dependent add per element. Splitting the
    // chain lets four run at once, which is worth 1.7x-3.5x depending on whether
    // the working set fits in cache -- and this is 95% of VectorIndex::search()
    // over a realistic corpus, local compute with no HTTP request to hide behind.
    //
    // It changes the summation order, and therefore the last bits of the result.
    // That is acceptable here and worth stating: these are similarity scores fed
    // to a ranking, not money.
    const double *x = a.constData();
    const double *y = b.constData();
    const qsizetype n = a.size();

    double s0 = 0.0;
    double s1 = 0.0;
    double s2 = 0.0;
    double s3 = 0.0;
    qsizetype i = 0;
    for (; i + 4 <= n; i += 4) {
        s0 += x[i] * y[i];
        s1 += x[i + 1] * y[i + 1];
        s2 += x[i + 2] * y[i + 2];
        s3 += x[i + 3] * y[i + 3];
    }
    double total = (s0 + s1) + (s2 + s3);
    for (; i < n; ++i)
        total += x[i] * y[i];
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

// Metric travels on the wire, so it gets one table and both directions derived
// from it, like the thirteen enums in Enums.cpp. It used to be a switch-shaped
// chain on encode and a separate if-chain on decode: three values, six
// spellings, no shared source. "cosine" in particular was written in exactly one
// place and read in none, being the fallback on both sides -- so renaming it on
// the encode side alone would have left every test passing over a file no other
// reader of this format would understand.
using detail::WireName;

constexpr WireName<VectorIndex::Metric> kMetrics[] = {
        {VectorIndex::Metric::Cosine, "cosine"},
        {VectorIndex::Metric::DotProduct, "dot_product"},
        {VectorIndex::Metric::Euclidean, "euclidean"},
};

struct Entry
{
    QList<double> vector;
    QString text;
    QJsonObject payload;
    // Cached at insertion. A cosine search needs it for every entry on every
    // query, and it cannot change without the vector changing.
    double norm = 0.0;
};

} // namespace

QTOPENAI_WIRE_CONVERSIONS(metricToString, metricFromString, VectorIndex::Metric, kMetrics,
                          VectorIndex::Metric::Cosine)

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
    d->entries.insert(id, Entry {vector, text, payload, Vector::norm(vector)});
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

    // The query's own length is the same for every entry it is compared with,
    // so it is measured once here rather than inside cosineSimilarity() once
    // per entry. With the entries' lengths cached at insertion, a cosine search
    // is left with the one dot product it genuinely has to compute per entry,
    // where it was doing three.
    const double queryNorm = d->metric == Metric::Cosine ? Vector::norm(query) : 0.0;

    // Scored candidates as a score and a position, so that ranking sorts two
    // numbers instead of shuffling ids, texts and payloads along with them --
    // and so that ties can be broken by position, which is what keeps entries
    // that score equally in insertion order.
    struct Candidate
    {
        double score;
        qsizetype position;
    };
    QList<Candidate> candidates;
    candidates.reserve(d->order.size());

    for (qsizetype position = 0; position < d->order.size(); ++position) {
        // constFind rather than value(): the latter returns by value, so
        // binding it to a reference still built and destroyed a whole Entry --
        // three implicit-sharing round-trips -- for every entry of every query.
        const auto it = d->entries.constFind(d->order.at(position));
        if (it == d->entries.constEnd())
            continue;
        const Entry &entry = it.value();

        double score = 0.0;
        switch (d->metric) {
        case Metric::Cosine: {
            // The same expression cosineSimilarity() evaluates, with both
            // lengths already known. A zero vector has no direction, so it is
            // not similar to anything -- an honest 0 rather than a division by
            // zero.
            const double denominator = queryNorm * entry.norm;
            score = denominator > 0.0 ? Vector::dot(query, entry.vector) / denominator : 0.0;
            break;
        }
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
        candidates.append(Candidate {score, position});
    }

    // Only the k best have to be in order, and k is a handful where the corpus
    // is not.
    const qsizetype wanted = qMin(qsizetype(k), candidates.size());
    std::partial_sort(candidates.begin(), candidates.begin() + wanted, candidates.end(),
                      [](const Candidate &a, const Candidate &b) {
                          if (a.score != b.score)
                              return a.score > b.score;
                          return a.position < b.position;
                      });

    matches.reserve(wanted);
    for (qsizetype i = 0; i < wanted; ++i) {
        const QString &id = d->order.at(candidates.at(i).position);
        const Entry &entry = d->entries.constFind(id).value();
        matches.append(VectorMatch {id, candidates.at(i).score, entry.text, entry.payload});
    }
    return matches;
}

QJsonObject VectorIndex::toJson() const
{
    QJsonArray entries;
    for (const QString &id : d->order) {
        const Entry &entry = d->entries.constFind(id).value();
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

    return {{QStringLiteral("metric"), metricToString(d->metric)},
            {QStringLiteral("entries"), entries}};
}

VectorIndex VectorIndex::fromJson(const QJsonObject &json)
{
    VectorIndex index;

    index.setMetric(metricFromString(json.value(QStringLiteral("metric")).toString()));

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
        const auto theirs = other.d->entries.constFind(id);
        if (theirs == other.d->entries.constEnd())
            return false;
        const Entry &mine = d->entries.constFind(id).value();
        if (mine.vector != theirs->vector || mine.text != theirs->text
            || mine.payload != theirs->payload)
            return false;
    }
    return true;
}

} // namespace Core
} // namespace QtOpenAi
