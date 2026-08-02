// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <limits>

namespace QtOpenAi {
namespace Core {

// Vector arithmetic over embeddings, on their own.
//
// Embeddings are QList<double> everywhere in this library, so these take that
// and nothing else. Mismatched lengths return 0 rather than reading past the
// shorter one -- comparing a 1536-dimension vector with a 3072-dimension one is
// a mistake, and the useful answer to a mistake is "no similarity".
namespace Vector {

QTOPENAI_CORE_EXPORT double dot(const QList<double> &a, const QList<double> &b);
QTOPENAI_CORE_EXPORT double norm(const QList<double> &vector);
// -1 to 1, and the metric to reach for by default: embedding models encode
// meaning in direction, and magnitude mostly encodes how long the text was.
QTOPENAI_CORE_EXPORT double cosineSimilarity(const QList<double> &a, const QList<double> &b);
QTOPENAI_CORE_EXPORT double euclideanDistance(const QList<double> &a, const QList<double> &b);
// Unit length, or the input unchanged if it has no length to normalise.
QTOPENAI_CORE_EXPORT QList<double> normalized(const QList<double> &vector);

} // namespace Vector

// One search hit.
struct QTOPENAI_CORE_EXPORT VectorMatch
{
    QString id;
    // What the ranking is on: always best-first, always "higher is better".
    // For a distance metric this is the *negated* distance, so code that ranks
    // or thresholds never has to ask which metric produced it.
    double score = 0.0;
    QString text;
    QJsonObject payload;
};

class VectorIndexData;

// A small in-memory vector index: the local, dependency-free half of
// retrieval-augmented generation.
//
//     VectorIndex index;
//     index.add("doc-1", embedding, "The cat sat on the mat.");
//     const auto hits = index.search(queryEmbedding, 5);
//
// It is a brute-force scan, and that is a deliberate choice rather than a gap.
// An approximate-nearest-neighbour structure earns its complexity somewhere
// past a hundred thousand vectors; below that a scan over a few thousand
// embeddings is a few milliseconds, and it is exact, has no index to rebuild
// and no parameters to tune wrongly. Past that point the answer is a real
// vector database, not a worse one here -- which is what OpenAI's own
// server-side vector stores are for.
//
// An implicitly-shared value type, so it can be copied, held in a model, and
// round-tripped through JSON without ceremony.
class QTOPENAI_CORE_EXPORT VectorIndex
{
public:
    // How similarity is measured.
    enum class Metric {
        Cosine,     // direction only; the default and almost always the right one
        DotProduct, // direction and magnitude; for already-normalised vectors
        Euclidean   // straight-line distance, ranked as its negation
    };

    VectorIndex();
    VectorIndex(const VectorIndex &other);
    VectorIndex(VectorIndex &&other) noexcept;
    VectorIndex &operator=(const VectorIndex &other);
    VectorIndex &operator=(VectorIndex &&other) noexcept;
    ~VectorIndex();

    void swap(VectorIndex &other) noexcept { d.swap(other.d); }

    // Add or replace an entry. Returns false, changing nothing, if the vector
    // is empty or its length differs from what the index already holds:
    // silently comparing vectors of different dimension produces rankings that
    // look plausible and are meaningless, and a model change mid-corpus is
    // exactly how that happens.
    bool add(const QString &id, const QList<double> &vector, const QString &text = QString(),
             const QJsonObject &payload = QJsonObject());
    bool remove(const QString &id);
    void clear();

    bool contains(const QString &id) const;
    int size() const;
    bool isEmpty() const;
    // The length every vector in this index has, or 0 while it is empty.
    int dimension() const;

    QStringList ids() const;
    QList<double> vector(const QString &id) const;
    QString text(const QString &id) const;
    QJsonObject payload(const QString &id) const;

    Metric metric() const;
    void setMetric(Metric metric);

    // The `k` best matches, best first. `minScore` drops anything below it --
    // for cosine, the difference between "the five closest documents" and "the
    // five closest documents that are actually about this", which for a
    // retrieval prompt is the difference between context and noise.
    QList<VectorMatch> search(const QList<double> &query, int k = 5,
                              double minScore = -std::numeric_limits<double>::infinity()) const;

    // Round-trips the whole index, so an application can build it once and load
    // it on the next run instead of paying to embed the same corpus again.
    QJsonObject toJson() const;
    static VectorIndex fromJson(const QJsonObject &json);

    bool operator==(const VectorIndex &other) const;
    bool operator!=(const VectorIndex &other) const { return !(*this == other); }

private:
    QSharedDataPointer<VectorIndexData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::VectorIndex)
Q_DECLARE_METATYPE(QtOpenAi::Core::VectorMatch)
