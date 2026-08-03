// SPDX-License-Identifier: MIT
#include <QtOpenAi/Tools/UtilityTools.h>

#include <QtCore/QDateTime>
#include <QtTest/QtTest>

using namespace QtOpenAi::Tools;

// Coverage for the harmless tools (#53).
class TestUtilityTools : public QObject
{
    Q_OBJECT
private slots:
    void theClockIsUnambiguous();
    void arithmeticIsCorrect();
    void precedenceAndAssociativity();
    void functionsAndConstants();
    void badExpressionsSayWhyRatherThanGuess();
    void thereIsNothingToEscapeInto();
    void uuidsAreUnique();
};

void TestUtilityTools::theClockIsUnambiguous()
{
    UtilityTools tools;
    const QDateTime parsed = QDateTime::fromString(tools.current_time(), Qt::ISODate);
    QVERIFY(parsed.isValid());
    // UTC: a local time without an offset is a time nobody can convert, and the
    // model has no way to ask which zone it was.
    QCOMPARE(parsed.timeSpec(), Qt::UTC);
    QVERIFY(qAbs(parsed.secsTo(QDateTime::currentDateTimeUtc())) < 5);
}

void TestUtilityTools::arithmeticIsCorrect()
{
    UtilityTools tools;
    QCOMPARE(tools.calculate(QStringLiteral("2 + 2")), QStringLiteral("4"));
    QCOMPARE(tools.calculate(QStringLiteral("10 / 4")), QStringLiteral("2.5"));
    QCOMPARE(tools.calculate(QStringLiteral("7 % 3")), QStringLiteral("1"));
    QCOMPARE(tools.calculate(QStringLiteral("-5 + 3")), QStringLiteral("-2"));
    QCOMPARE(tools.calculate(QStringLiteral("--5")), QStringLiteral("5"));
    QCOMPARE(tools.calculate(QStringLiteral("1e3 + 1")), QStringLiteral("1001"));
    QCOMPARE(tools.calculate(QStringLiteral("1.5e-2")), QStringLiteral("0.015"));
    // The reason this tool exists: a model doing this by predicting the next
    // token gets it wrong in a way that looks right.
    QCOMPARE(tools.calculate(QStringLiteral("98765 * 43210")), QStringLiteral("4267635650"));
    // Not "-0".
    QCOMPARE(tools.calculate(QStringLiteral("0 * -1")), QStringLiteral("0"));
}

void TestUtilityTools::precedenceAndAssociativity()
{
    UtilityTools tools;
    QCOMPARE(tools.calculate(QStringLiteral("2 + 3 * 4")), QStringLiteral("14"));
    QCOMPARE(tools.calculate(QStringLiteral("(2 + 3) * 4")), QStringLiteral("20"));
    QCOMPARE(tools.calculate(QStringLiteral("10 - 3 - 2")), QStringLiteral("5"));
    QCOMPARE(tools.calculate(QStringLiteral("100 / 10 / 2")), QStringLiteral("5"));
    // Right-associative, as everyone writes it: 2^3^2 is 512, not 64.
    QCOMPARE(tools.calculate(QStringLiteral("2 ^ 3 ^ 2")), QStringLiteral("512"));
    // Unary minus binds looser than '^', as in mathematics and in Python.
    QCOMPARE(tools.calculate(QStringLiteral("-2 ^ 2")), QStringLiteral("-4"));
    QCOMPARE(tools.calculate(QStringLiteral("(-2) ^ 2")), QStringLiteral("4"));
    QCOMPARE(tools.calculate(QStringLiteral("2 ^ -3")), QStringLiteral("0.125"));
    QCOMPARE(tools.calculate(QStringLiteral("((1 + 2) * (3 + 4)) - 1")), QStringLiteral("20"));
}

void TestUtilityTools::functionsAndConstants()
{
    UtilityTools tools;
    QCOMPARE(tools.calculate(QStringLiteral("sqrt(16)")), QStringLiteral("4"));
    QCOMPARE(tools.calculate(QStringLiteral("abs(-3.5)")), QStringLiteral("3.5"));
    QCOMPARE(tools.calculate(QStringLiteral("min(3, 7)")), QStringLiteral("3"));
    QCOMPARE(tools.calculate(QStringLiteral("max(3, 7)")), QStringLiteral("7"));
    QCOMPARE(tools.calculate(QStringLiteral("round(2.6)")), QStringLiteral("3"));
    QCOMPARE(tools.calculate(QStringLiteral("floor(2.9)")), QStringLiteral("2"));
    QCOMPARE(tools.calculate(QStringLiteral("ceil(2.1)")), QStringLiteral("3"));
    QCOMPARE(tools.calculate(QStringLiteral("SQRT(9)")), QStringLiteral("3")); // case-insensitive
    QVERIFY(tools.calculate(QStringLiteral("pi")).startsWith(QStringLiteral("3.14159")));
    QCOMPARE(tools.calculate(QStringLiteral("round(pi * 100) / 100")), QStringLiteral("3.14"));
}

void TestUtilityTools::badExpressionsSayWhyRatherThanGuess()
{
    UtilityTools tools;
    // A tool that returns a plausible number for a broken expression is worse
    // than one that says it could not: the model has no way to notice.
    QVERIFY(tools.calculate(QStringLiteral("1 / 0")).contains(QStringLiteral("division by zero")));
    QVERIFY(tools.calculate(QStringLiteral("5 % 0")).contains(QStringLiteral("division by zero")));
    QVERIFY(tools.calculate(QStringLiteral("(1 + 2")).contains(QStringLiteral("not closed")));
    QVERIFY(tools.calculate(QStringLiteral("1 +")).contains(QStringLiteral("cannot evaluate")));
    QVERIFY(tools.calculate(QStringLiteral("1 2")).contains(QStringLiteral("unexpected")));
    QVERIFY(tools.calculate(QStringLiteral("sqrt(-1)")).contains(QStringLiteral("not real")));
    QVERIFY(tools.calculate(QStringLiteral("min(1)")).contains(QStringLiteral("2 argument")));
    QVERIFY(tools.calculate(QString()).contains(QStringLiteral("no expression")));
    // An overflow is reported rather than returned as "inf".
    QVERIFY(tools.calculate(QStringLiteral("1e308 * 1e308"))
                    .contains(QStringLiteral("not a finite number")));
}

void TestUtilityTools::thereIsNothingToEscapeInto()
{
    // The reason this is a parser and not a scripting engine. Every one of
    // these is an expression a model could be talked into producing, and every
    // one of them is a syntax error here rather than a capability.
    UtilityTools tools;
    const QStringList attempts
            = {QStringLiteral("process.exit(1)"),     QStringLiteral("require('fs')"),
               QStringLiteral("globalThis"),          QStringLiteral("(function(){return 1})()"),
               QStringLiteral("1; console.log('x')"), QStringLiteral("[].constructor"),
               QStringLiteral("__import__('os')"),    QStringLiteral("Qt.quit()")};
    for (const QString &attempt : attempts) {
        const QString result = tools.calculate(attempt);
        QVERIFY2(result.startsWith(QStringLiteral("cannot evaluate")), qPrintable(attempt));
    }
}

void TestUtilityTools::uuidsAreUnique()
{
    UtilityTools tools;
    const QString first = tools.uuid();
    QCOMPARE(first.length(), 36); // no braces
    QVERIFY(!first.contains(QLatin1Char('{')));
    QVERIFY(first != tools.uuid());

    QCOMPARE(UtilityTools::toolNames().size(), 3);
}

QTEST_MAIN(TestUtilityTools)
#include "tst_utility.moc"
