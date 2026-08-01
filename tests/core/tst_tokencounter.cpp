// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/ModelCatalog.h>
#include <QtOpenAi/Core/TokenCounter.h>

#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

namespace {

// A vocabulary small enough that the expected merges can be worked out by hand,
// which is what makes this a test of the algorithm rather than of a data file.
//
//   "a" 0   "b" 1   "ab" 2   "aba" 3   " " 4
//
// Encoding "ababa" then has exactly one answer. Every adjacent pair is scored
// and the lowest-ranked one merges first:
//
//   a b a b a  -> "ab"(2) is the lowest        -> ab a b a
//   ab a b a   -> "ab"(2) again, over "aba"(3) -> ab ab a
//   ab ab a    -> only "aba"(3) is left        -> ab aba
//
// leaving {2, 3}. Taking the highest-ranked pair first, or working left to
// right, would give a different answer -- which is the point.
QByteArray testVocabulary()
{
    const QList<QPair<QByteArray, int>> tokens {
            {"a", 0}, {"b", 1}, {"ab", 2}, {"aba", 3}, {" ", 4},
    };

    QByteArray data;
    for (const auto &token : tokens)
        data += token.first.toBase64() + ' ' + QByteArray::number(token.second) + '\n';
    return data;
}

// Splitting on runs of letters and runs of spaces, so the piece boundaries in
// these tests are obvious by inspection.
const QString kTestPattern = QStringLiteral("[a-z]+|\\s+");

} // namespace

// Coverage for local token counting (#44). No vocabulary is bundled, so these
// load a hand-built one -- the algorithm is what this library ships, and the
// data is what the caller supplies.
class TestTokenCounter : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();

    void estimatesWhenNoVocabularyIsLoaded();
    void mergesTheLowestRankedPairFirst();
    void splitsWithTheEncodingsPattern();
    void countsBytesOutsideTheVocabulary();
    void countsMessageFraming();
    void becomesExactWhenItsVocabularyArrives();
    void rejectsDataItCannotRead();
    void takesTheEncodingFromTheCatalog();
};

void TestTokenCounter::initTestCase()
{
    QVERIFY(TokenCounter::loadEncoding(QStringLiteral("test_base"), testVocabulary(),
                                       kTestPattern));
    QVERIFY(TokenCounter::hasEncoding(QStringLiteral("test_base")));
    QVERIFY(TokenCounter::loadedEncodings().contains(QStringLiteral("test_base")));
}

void TestTokenCounter::estimatesWhenNoVocabularyIsLoaded()
{
    const TokenCounter counter;

    QVERIFY(!counter.isExact());
    QCOMPARE(counter.count(QString()), 0);
    // The customary one-token-per-four-characters estimate, rounded up so that
    // any text at all costs at least one token.
    QCOMPARE(counter.count(QStringLiteral("a")), 1);
    QCOMPARE(counter.count(QStringLiteral("abcd")), 1);
    QCOMPARE(counter.count(QStringLiteral("abcde")), 2);
    // Without a vocabulary there are no tokens to name, only a number.
    QVERIFY(counter.encode(QStringLiteral("abcde")).isEmpty());
}

void TestTokenCounter::mergesTheLowestRankedPairFirst()
{
    const TokenCounter counter(QStringLiteral("test_base"));
    QVERIFY(counter.isExact());

    // A piece that is a token already needs no merging.
    QCOMPARE(counter.encode(QStringLiteral("ab")), QList<int>({2}));
    // ... and one that is not is merged in rank order; see the comment above.
    QCOMPARE(counter.encode(QStringLiteral("ababa")), QList<int>({2, 3}));
    QCOMPARE(counter.count(QStringLiteral("ababa")), 2);
}

void TestTokenCounter::splitsWithTheEncodingsPattern()
{
    const TokenCounter counter(QStringLiteral("test_base"));

    // Merging never crosses a piece boundary: "ab ab" cannot become one token
    // even though its letters could.
    QCOMPARE(counter.encode(QStringLiteral("ab ab")), QList<int>({2, 4, 2}));
}

void TestTokenCounter::countsBytesOutsideTheVocabulary()
{
    const TokenCounter counter(QStringLiteral("test_base"));

    // A byte this vocabulary cannot name still costs a token, so a count is
    // never silently short.
    QCOMPARE(counter.count(QStringLiteral("zzz")), 3);
    QCOMPARE(counter.encode(QStringLiteral("zzz")), QList<int>({-1, -1, -1}));
}

void TestTokenCounter::countsMessageFraming()
{
    const TokenCounter counter(QStringLiteral("test_base"));

    QCOMPARE(counter.count(QList<Message>()), 0);

    // Each message is wrapped in role and separator tokens, and the reply is
    // primed with three more:
    //   3 (framing) + 4 ("user", no token for it here) + 1 ("ab") + 3 (priming)
    QCOMPARE(counter.count(QStringLiteral("user")), 4);
    QCOMPARE(counter.count({Message(Role::User, QStringLiteral("ab"))}), 11);

    // A name costs its own tokens plus the field that carries it.
    Message named(Role::User, QStringLiteral("ab"));
    named.setName(QStringLiteral("ab"));
    QCOMPARE(counter.count({named}), 11 + 1 + 1);
}

void TestTokenCounter::becomesExactWhenItsVocabularyArrives()
{
    // A counter is often built at start-up, before the data file has been
    // found; it must not be stuck on the estimate for the rest of the run.
    const TokenCounter counter(QStringLiteral("late_base"));
    QVERIFY(!counter.isExact());
    QCOMPARE(counter.count(QStringLiteral("ababa")), 2); // heuristic: 5 / 4

    QVERIFY(TokenCounter::loadEncoding(QStringLiteral("late_base"), testVocabulary(),
                                       kTestPattern));

    QVERIFY(counter.isExact());
    QCOMPARE(counter.encode(QStringLiteral("ababa")), QList<int>({2, 3}));
}

void TestTokenCounter::rejectsDataItCannotRead()
{
    QVERIFY(!TokenCounter::loadEncoding(QStringLiteral("empty_base"), QByteArray()));
    QVERIFY(!TokenCounter::loadEncoding(QStringLiteral("empty_base"),
                                        QByteArray("not a vocabulary at all\n")));
    QVERIFY(!TokenCounter::hasEncoding(QStringLiteral("empty_base")));

    QVERIFY(!TokenCounter::loadEncodingFile(QStringLiteral("missing_base"),
                                            QStringLiteral("/does/not/exist.tiktoken")));
    QVERIFY(!TokenCounter::hasEncoding(QStringLiteral("missing_base")));
}

void TestTokenCounter::takesTheEncodingFromTheCatalog()
{
    // The catalog already knows which tokenizer a model uses, so nobody has to
    // repeat the mapping.
    QCOMPARE(TokenCounter::forModel(QStringLiteral("gpt-4o-mini")).encoding(),
             QStringLiteral("o200k_base"));
    QCOMPARE(TokenCounter::forModel(QStringLiteral("gpt-3.5-turbo")).encoding(),
             QStringLiteral("cl100k_base"));
    // Even for a model it has never heard of, so the counter still works.
    QVERIFY(!TokenCounter::forModel(QStringLiteral("something-else")).encoding().isEmpty());
}

QTEST_MAIN(TestTokenCounter)
#include "tst_tokencounter.moc"
