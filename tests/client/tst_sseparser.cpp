// SPDX-License-Identifier: MIT
#include "SseParser_p.h"

#include <QtTest/QtTest>

using QtOpenAi::Client::detail::SseEvent;
using QtOpenAi::Client::detail::SseParser;

// The parser is a private header, exercised end-to-end by the streaming tests
// over a stub server. What those cannot do is choose where a chunk boundary
// falls, and the incremental scan is defined entirely by that: it remembers how
// far it has already looked, so a bug there only shows when a separator lands
// across a feed.
class TestSseParser : public QObject
{
    Q_OBJECT

private slots:
    void framesWholeBody();
    void framesAcrossChunkBoundary_data();
    void framesAcrossChunkBoundary();
    void keepsNameAndConcatenatesData();
};

static QList<QByteArray> payloads(const QList<SseEvent> &events)
{
    QList<QByteArray> out;
    for (const SseEvent &event : events)
        out.append(event.data);
    return out;
}

void TestSseParser::framesWholeBody()
{
    SseParser parser;
    const QList<SseEvent> events = parser.feed("data: a\n\ndata: b\n\n");
    QCOMPARE(payloads(events), QList<QByteArray>({"a", "b"}));
    QVERIFY(parser.buffered().isEmpty());
}

void TestSseParser::framesAcrossChunkBoundary_data()
{
    QTest::addColumn<QList<QByteArray>>("chunks");

    // Every place a two-event LF stream can be cut in two.
    const QByteArray lf = "data: a\n\ndata: b\n\n";
    for (qsizetype i = 1; i < lf.size(); ++i)
        QTest::addRow("lf split at %lld", qlonglong(i))
                << QList<QByteArray>({lf.left(i), lf.mid(i)});

    // The same for a CRLF stream, where each feed is also rewritten in place.
    // The cut inside the final "\r\n\r\n" of the first event is the one that
    // matters: normalising drops a byte *before* the point the previous feed
    // had scanned up to, which moves a separator back underneath it.
    const QByteArray crlf = "data: a\r\n\r\ndata: b\r\n\r\n";
    for (qsizetype i = 1; i < crlf.size(); ++i)
        QTest::addRow("crlf split at %lld", qlonglong(i))
                << QList<QByteArray>({crlf.left(i), crlf.mid(i)});

    // Byte at a time, which puts every boundary in play at once.
    QList<QByteArray> single;
    for (const char c : crlf)
        single.append(QByteArray(1, c));
    QTest::newRow("crlf one byte per feed") << single;
}

void TestSseParser::framesAcrossChunkBoundary()
{
    QFETCH(QList<QByteArray>, chunks);

    SseParser parser;
    QList<SseEvent> events;
    for (const QByteArray &chunk : chunks)
        events += parser.feed(chunk);

    QCOMPARE(payloads(events), QList<QByteArray>({"a", "b"}));
    QVERIFY(parser.buffered().isEmpty());
}

void TestSseParser::keepsNameAndConcatenatesData()
{
    SseParser parser;
    QList<SseEvent> events = parser.feed(": keep-alive\nevent: thread.run.completed\r\n");
    QVERIFY(events.isEmpty());
    events = parser.feed("data: {\"id\":\ndata: 1}\r\n\r\n");

    QCOMPARE(events.size(), 1);
    QCOMPARE(events.at(0).name, QByteArray("thread.run.completed"));
    QCOMPARE(events.at(0).data, QByteArray("{\"id\":1}"));
}

QTEST_MAIN(TestSseParser)
#include "tst_sseparser.moc"
