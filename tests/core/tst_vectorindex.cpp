// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/VectorIndex.h>

#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the local vector index (#51).
class TestVectorIndex : public QObject
{
    Q_OBJECT
private slots:
    void theMathIsTheMath();
    void mismatchedLengthsAreNotComparable();
    void rankingIsCorrectForKnownVectors();
    void topKAndTheScoreFloor();
    void equalScoresKeepInsertionOrder();
    void aReplacedVectorIsRankedByItsNewLength();
    void everyMetricRanksBestFirst();
    void entriesCanBeReplacedAndRemoved();
    void aMixedDimensionIsRefused();
    void itRoundTripsThroughJson();
};

void TestVectorIndex::theMathIsTheMath()
{
    const QList<double> x {1.0, 0.0, 0.0};
    const QList<double> y {0.0, 1.0, 0.0};

    QCOMPARE(Vector::dot(x, x), 1.0);
    QCOMPARE(Vector::dot(x, y), 0.0);
    QCOMPARE(Vector::norm({3.0, 4.0}), 5.0);

    // Orthogonal is 0, identical is 1, opposite is -1.
    QCOMPARE(Vector::cosineSimilarity(x, y), 0.0);
    QCOMPARE(Vector::cosineSimilarity(x, x), 1.0);
    QCOMPARE(Vector::cosineSimilarity(x, {-1.0, 0.0, 0.0}), -1.0);
    // Direction only: length must not change the answer.
    QCOMPARE(Vector::cosineSimilarity(x, {5.0, 0.0, 0.0}), 1.0);

    QCOMPARE(Vector::euclideanDistance({0.0, 0.0}, {3.0, 4.0}), 5.0);

    const QList<double> unit = Vector::normalized({3.0, 4.0});
    QVERIFY(qFuzzyCompare(Vector::norm(unit), 1.0));
    // A zero vector has no direction to normalise, so it comes back unchanged
    // rather than as a list of NaN.
    QCOMPARE(Vector::normalized({0.0, 0.0}), QList<double>({0.0, 0.0}));
    QCOMPARE(Vector::cosineSimilarity({0.0, 0.0}, x), 0.0);
}

void TestVectorIndex::mismatchedLengthsAreNotComparable()
{
    // Reading past the shorter one would give a number, and the number would
    // be meaningless. 0 says "not similar", which is the useful answer.
    QCOMPARE(Vector::dot({1.0, 2.0}, {1.0, 2.0, 3.0}), 0.0);
    QCOMPARE(Vector::cosineSimilarity({1.0, 2.0}, {1.0, 2.0, 3.0}), 0.0);
    QCOMPARE(Vector::euclideanDistance({1.0}, {1.0, 2.0}), 0.0);
}

void TestVectorIndex::rankingIsCorrectForKnownVectors()
{
    VectorIndex index;
    QVERIFY(index.isEmpty());
    QCOMPARE(index.dimension(), 0);

    QVERIFY(index.add(QStringLiteral("east"), {1.0, 0.0}, QStringLiteral("towards the sunrise")));
    QVERIFY(index.add(QStringLiteral("north"), {0.0, 1.0}));
    QVERIFY(index.add(QStringLiteral("west"), {-1.0, 0.0}));
    QVERIFY(index.add(QStringLiteral("north-east"), {0.7, 0.7}));

    QCOMPARE(index.size(), 4);
    QCOMPARE(index.dimension(), 2);
    QVERIFY(index.contains(QStringLiteral("east")));
    QCOMPARE(index.text(QStringLiteral("east")), QStringLiteral("towards the sunrise"));

    // Asking due east: east, then north-east, then north, then west.
    const QList<VectorMatch> hits = index.search({1.0, 0.0}, 4);
    QCOMPARE(hits.size(), 4);
    QCOMPARE(hits.at(0).id, QStringLiteral("east"));
    QCOMPARE(hits.at(1).id, QStringLiteral("north-east"));
    QCOMPARE(hits.at(2).id, QStringLiteral("north"));
    QCOMPARE(hits.at(3).id, QStringLiteral("west"));

    // Best first means monotonically non-increasing, not merely "the top one
    // is right".
    for (int i = 1; i < hits.size(); ++i)
        QVERIFY(hits.at(i - 1).score >= hits.at(i).score);

    QCOMPARE(hits.at(0).score, 1.0);
    QCOMPARE(hits.at(3).score, -1.0);
    // The payload rides along, so a hit is usable without a second lookup.
    QCOMPARE(hits.at(0).text, QStringLiteral("towards the sunrise"));
}

void TestVectorIndex::topKAndTheScoreFloor()
{
    VectorIndex index;
    index.add(QStringLiteral("a"), {1.0, 0.0});
    index.add(QStringLiteral("b"), {0.9, 0.1});
    index.add(QStringLiteral("c"), {0.0, 1.0});
    index.add(QStringLiteral("d"), {-1.0, 0.0});

    QCOMPARE(index.search({1.0, 0.0}, 2).size(), 2);
    // k larger than the index is not an error; it is everything.
    QCOMPARE(index.search({1.0, 0.0}, 99).size(), 4);
    QVERIFY(index.search({1.0, 0.0}, 0).isEmpty());

    // The floor is the difference between "the closest documents" and "the
    // closest documents that are actually about this".
    const QList<VectorMatch> relevant = index.search({1.0, 0.0}, 10, 0.5);
    QCOMPARE(relevant.size(), 2);
    QCOMPARE(relevant.at(0).id, QStringLiteral("a"));
    QCOMPARE(relevant.at(1).id, QStringLiteral("b"));

    // Searching an empty index, or with an empty query, is empty rather than
    // undefined.
    QVERIFY(VectorIndex().search({1.0, 0.0}, 5).isEmpty());
    QVERIFY(index.search({}, 5).isEmpty());
}

void TestVectorIndex::equalScoresKeepInsertionOrder()
{
    // A ranking that reshuffles equal scores between runs is a ranking nobody
    // can test. Only the k best are put in order now, so the tie-break has to
    // be part of the comparison rather than a property of sorting everything.
    VectorIndex index;
    for (int i = 0; i < 12; ++i) {
        // Same direction, different lengths: cosine scores every one of them
        // exactly 1, so nothing but insertion order can separate them.
        index.add(QStringLiteral("tie%1").arg(i), {double(i + 1), 0.0});
    }
    index.add(QStringLiteral("other"), {0.0, 1.0});

    for (const int k : {1, 3, 12, 13, 99}) {
        const QList<VectorMatch> hits = index.search({1.0, 0.0}, k);
        const int tied = qMin(k, 12);
        QCOMPARE(hits.size(), qMin(k, 13));
        for (int i = 0; i < tied; ++i) {
            QCOMPARE(hits.at(i).id, QStringLiteral("tie%1").arg(i));
            QCOMPARE(hits.at(i).score, 1.0);
        }
    }
}

void TestVectorIndex::aReplacedVectorIsRankedByItsNewLength()
{
    // Each entry's length is measured when it goes in rather than on every
    // query, so replacing an entry has to measure it again -- a stale length
    // would score the new vector as if it were still the old one.
    VectorIndex index;
    index.add(QStringLiteral("a"), {3.0, 4.0}); // length 5, points up-right
    index.add(QStringLiteral("b"), {0.0, 1.0});

    QVERIFY(index.add(QStringLiteral("a"), {100.0, 0.0})); // length 100, points right
    const QList<VectorMatch> hits = index.search({1.0, 0.0}, 2);
    QCOMPARE(hits.at(0).id, QStringLiteral("a"));
    // Cosine ignores magnitude, so a vector straight along the query is 1 --
    // which it is only if the length used is the new one.
    QCOMPARE(hits.at(0).score, 1.0);

    // The same holds for an index rebuilt from JSON, which goes in through the
    // same door.
    const VectorIndex reloaded = VectorIndex::fromJson(index.toJson());
    QCOMPARE(reloaded.search({1.0, 0.0}, 1).at(0).score, 1.0);
}

void TestVectorIndex::everyMetricRanksBestFirst()
{
    VectorIndex index;
    index.add(QStringLiteral("near"), {1.0, 0.1});
    index.add(QStringLiteral("far"), {-3.0, 4.0});
    index.add(QStringLiteral("long-same-direction"), {10.0, 1.0});

    // Cosine ignores magnitude, so the long vector pointing the same way is as
    // good as the short one.
    index.setMetric(VectorIndex::Metric::Cosine);
    QCOMPARE(index.metric(), VectorIndex::Metric::Cosine);
    const QList<VectorMatch> cosine = index.search({1.0, 0.1}, 3);
    QCOMPARE(cosine.at(0).id, QStringLiteral("near"));
    QCOMPARE(cosine.at(2).id, QStringLiteral("far"));

    // Dot product does not, so length wins.
    index.setMetric(VectorIndex::Metric::DotProduct);
    QCOMPARE(index.search({1.0, 0.1}, 1).at(0).id, QStringLiteral("long-same-direction"));

    // Euclidean is a distance, and it is still reported so that higher is
    // better -- callers never have to branch on which metric produced a score.
    index.setMetric(VectorIndex::Metric::Euclidean);
    const QList<VectorMatch> euclidean = index.search({1.0, 0.1}, 3);
    QCOMPARE(euclidean.at(0).id, QStringLiteral("near"));
    QCOMPARE(euclidean.at(0).score, 0.0); // it is the query
    for (int i = 1; i < euclidean.size(); ++i) {
        QVERIFY(euclidean.at(i).score < 0.0);
        QVERIFY(euclidean.at(i - 1).score >= euclidean.at(i).score);
    }
}

void TestVectorIndex::entriesCanBeReplacedAndRemoved()
{
    VectorIndex index;
    index.add(QStringLiteral("a"), {1.0, 0.0}, QStringLiteral("first"));
    index.add(QStringLiteral("b"), {0.0, 1.0});

    // Same id, new content: replaced, not duplicated.
    QVERIFY(index.add(QStringLiteral("a"), {0.5, 0.5}, QStringLiteral("second")));
    QCOMPARE(index.size(), 2);
    QCOMPARE(index.text(QStringLiteral("a")), QStringLiteral("second"));
    QCOMPARE(index.vector(QStringLiteral("a")), QList<double>({0.5, 0.5}));
    // Insertion order survives a replacement, so results stay reproducible.
    QCOMPARE(index.ids(), QStringList({QStringLiteral("a"), QStringLiteral("b")}));

    QVERIFY(index.remove(QStringLiteral("a")));
    QVERIFY(!index.remove(QStringLiteral("a")));
    QCOMPARE(index.ids(), QStringList({QStringLiteral("b")}));

    // An id or a vector that says nothing is refused rather than stored.
    QVERIFY(!index.add(QString(), {1.0, 0.0}));
    QVERIFY(!index.add(QStringLiteral("c"), {}));
    QCOMPARE(index.size(), 1);

    index.clear();
    QVERIFY(index.isEmpty());
    QCOMPARE(index.dimension(), 0);
}

void TestVectorIndex::aMixedDimensionIsRefused()
{
    // Vectors from two different embedding models rank against each other as
    // convincing nonsense, and a model change mid-corpus is exactly how that
    // happens. Refusing is the only way the caller finds out.
    VectorIndex index;
    QVERIFY(index.add(QStringLiteral("a"), {1.0, 0.0, 0.0}));
    QVERIFY(!index.add(QStringLiteral("b"), {1.0, 0.0}));
    QVERIFY(!index.contains(QStringLiteral("b")));
    QCOMPARE(index.size(), 1);
    QCOMPARE(index.dimension(), 3);

    // Emptying it takes any dimension again -- which is what makes
    // clear()-then-reindex with a different model work.
    index.clear();
    QVERIFY(index.add(QStringLiteral("b"), {1.0, 0.0}));
    QCOMPARE(index.dimension(), 2);

    // Removing the last entry does the same.
    index.remove(QStringLiteral("b"));
    QVERIFY(index.add(QStringLiteral("c"), {1.0, 2.0, 3.0, 4.0}));
    QCOMPARE(index.dimension(), 4);
}

void TestVectorIndex::itRoundTripsThroughJson()
{
    // The reason it matters: an application builds the index once and loads it
    // on the next run rather than paying to embed the same corpus again.
    VectorIndex index;
    index.setMetric(VectorIndex::Metric::Euclidean);
    index.add(QStringLiteral("a"), {1.0, 0.25}, QStringLiteral("the first one"),
              QJsonObject {{QStringLiteral("page"), 7}});
    index.add(QStringLiteral("b"), {0.0, 1.0});

    const VectorIndex restored = VectorIndex::fromJson(index.toJson());
    QCOMPARE(restored, index);
    QCOMPARE(restored.metric(), VectorIndex::Metric::Euclidean);
    QCOMPARE(restored.ids(), index.ids());
    QCOMPARE(restored.vector(QStringLiteral("a")), QList<double>({1.0, 0.25}));
    QCOMPARE(restored.text(QStringLiteral("a")), QStringLiteral("the first one"));
    QCOMPARE(restored.payload(QStringLiteral("a")).value(QStringLiteral("page")).toInt(), 7);
    QCOMPARE(restored.dimension(), 2);
    // And it still ranks the same, which is the only thing a restored index is
    // for.
    QCOMPARE(restored.search({1.0, 0.25}, 1).at(0).id, QStringLiteral("a"));

    // A value type: copies compare equal and stay independent.
    VectorIndex copy = index;
    QCOMPARE(copy, index);
    copy.remove(QStringLiteral("a"));
    QVERIFY(copy != index);
    QVERIFY(index.contains(QStringLiteral("a")));

    QCOMPARE(VectorIndex::fromJson(QJsonObject()), VectorIndex());
}

QTEST_MAIN(TestVectorIndex)
#include "tst_vectorindex.moc"
