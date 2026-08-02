// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/ChatMap.h>
#include <QtOpenAi/Client/Client.h>

#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

namespace {

QByteArray completion(const QString &content)
{
    return QStringLiteral(R"({"id":"c","object":"chat.completion","created":1,
        "model":"m","choices":[{"index":0,"finish_reason":"stop",
        "message":{"role":"assistant","content":"%1"}}]})")
            .arg(content)
            .toUtf8();
}

QStringList prompts(int count)
{
    QStringList list;
    for (int i = 0; i < count; ++i)
        list.append(QStringLiteral("prompt %1").arg(i));
    return list;
}

// Echoes the prompt back as the answer, after holding the request open.
//
// Echoing is what makes "in order" checkable: with a fixed answer, a run that
// shuffled its results would pass. Holding is what makes "at most N at once"
// checkable: an immediate answer never overlaps with anything.
class EchoServer : public QObject
{
public:
    explicit EchoServer(int holdMs = 60, QObject *parent = nullptr)
        : QObject(parent)
        , m_holdMs(holdMs)
    {
        m_server.listen(QHostAddress::LocalHost, 0);
        connect(&m_server, &QTcpServer::newConnection, this, &EchoServer::onConnection);
    }

    QUrl baseUrl() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1/v1").arg(m_server.serverPort()));
    }

    // Answer these prompts with an HTTP error instead.
    void setFailing(const QSet<QString> &prompts) { m_failing = prompts; }

    int requestCount() const { return m_requests; }
    int peakConcurrency() const { return m_peak; }

private:
    void onConnection()
    {
        QTcpSocket *socket = m_server.nextPendingConnection();
        auto buffer = std::make_shared<QByteArray>();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer]() {
            *buffer += socket->readAll();
            const int headerEnd = buffer->indexOf("\r\n\r\n");
            if (headerEnd < 0 || m_answering.contains(socket))
                return;

            int contentLength = 0;
            for (const QByteArray &line : buffer->left(headerEnd).split('\n')) {
                const QByteArray trimmed = line.trimmed().toLower();
                if (trimmed.startsWith("content-length:"))
                    contentLength = trimmed.mid(15).trimmed().toInt();
            }
            if (buffer->size() < headerEnd + 4 + contentLength)
                return;

            m_answering.insert(socket);
            ++m_requests;
            ++m_inFlight;
            m_peak = qMax(m_peak, m_inFlight);

            const QString prompt = promptOf(buffer->mid(headerEnd + 4, contentLength));
            QTimer::singleShot(
                    m_holdMs, this, [this, prompt, socket = QPointer<QTcpSocket>(socket)]() {
                        --m_inFlight;
                        if (!socket)
                            return;
                        const bool fail = m_failing.contains(prompt);
                        const QByteArray body
                                = fail ? QByteArray(R"({"error":{"message":"refused"}})")
                                       : completion(prompt);
                        socket->write("HTTP/1.1 " + QByteArray::number(fail ? 400 : 200)
                                      + " OK\r\nContent-Type: application/json\r\nContent-Length: "
                                      + QByteArray::number(body.size())
                                      + "\r\nConnection: close\r\n\r\n" + body);
                        socket->flush();
                        socket->disconnectFromHost();
                    });
        });
    }

    static QString promptOf(const QByteArray &body)
    {
        const QJsonObject json = QJsonDocument::fromJson(body).object();
        const QJsonArray messages = json.value(QStringLiteral("messages")).toArray();
        return messages.isEmpty()
                       ? QString()
                       : messages.last().toObject().value(QStringLiteral("content")).toString();
    }

    QTcpServer m_server;
    QSet<QTcpSocket *> m_answering;
    QSet<QString> m_failing;
    int m_holdMs;
    int m_requests = 0;
    int m_inFlight = 0;
    int m_peak = 0;
};

bool settled(ChatMapReply *run, int timeoutMs = 15000)
{
    if (!run)
        return false;
    QSignalSpy finished(run, &ChatMapReply::allFinished);
    return run->isFinished() || finished.wait(timeoutMs);
}

} // namespace

// Coverage for mapping many prompts at once (#52).
class TestChatMap : public QObject
{
    Q_OBJECT
private slots:
    void resultsComeBackInInputOrder();
    void concurrencyIsCapped();
    void oneFailedItemDoesNotFailTheRun();
    void maxFailuresStopsARunThatIsAllFailing();
    void progressIsReportedPerItem();
    void anEmptyRunFinishesAtOnce();
    void abortStopsIssuingAndStillFinishes();
    void requestsCanDifferPerItem();
    void aMapWithoutAClientFailsEveryItem();
};

void TestChatMap::resultsComeBackInInputOrder()
{
    // The whole point: classifying a thousand rows is only useful if row 837's
    // answer is still at 837. The server holds requests open and answers out of
    // order by construction, so an implementation that appended as it went
    // would fail here.
    EchoServer server(40);
    Client client;
    client.setBaseUrl(server.baseUrl());

    ChatMap map(&client);
    map.setConcurrency(4);

    auto *run = map.map(QStringLiteral("m"), prompts(9));
    QCOMPARE(run->count(), 9);
    // Index-aligned from the first moment, not only once everything answered.
    QCOMPARE(run->results().size(), 9);
    QCOMPARE(run->result(3).index, 3);
    QVERIFY(!run->result(3).finished);

    QVERIFY(settled(run));
    QVERIFY(run->isFinished());
    QCOMPARE(run->successCount(), 9);
    QCOMPARE(run->failureCount(), 0);
    QCOMPARE(run->contents(), prompts(9));
}

void TestChatMap::concurrencyIsCapped()
{
    EchoServer server(60);
    Client client;
    client.setBaseUrl(server.baseUrl());

    ChatMap map(&client);
    QCOMPARE(map.concurrency(), 4);
    map.setConcurrency(3);

    auto *run = map.map(QStringLiteral("m"), prompts(10));
    QVERIFY(settled(run));

    // Measured at the server, not in the runner's own bookkeeping.
    QVERIFY2(server.peakConcurrency() <= 3,
             qPrintable(QStringLiteral("peak was %1").arg(server.peakConcurrency())));
    // Capped, not batched: 10 items at 3 at a time is 10 requests, and a
    // runner that waited for each group of 3 would still pass the check above.
    QCOMPARE(server.requestCount(), 10);
    QCOMPARE(run->successCount(), 10);

    // A concurrency below one would issue nothing at all.
    map.setConcurrency(0);
    QCOMPARE(map.concurrency(), 1);
}

void TestChatMap::oneFailedItemDoesNotFailTheRun()
{
    // One row hitting a content filter is not a reason to throw away the rest.
    EchoServer server(10);
    server.setFailing({QStringLiteral("prompt 2"), QStringLiteral("prompt 5")});

    Client client;
    client.setBaseUrl(server.baseUrl());
    client.setRetryPolicy(RetryPolicy::none());

    ChatMap map(&client);
    auto *run = map.map(QStringLiteral("m"), prompts(7));
    QVERIFY(settled(run));

    QCOMPARE(run->successCount(), 5);
    QCOMPARE(run->failureCount(), 2);
    QCOMPARE(run->finishedCount(), 7);

    // The errors are at their own indices, and the neighbours are unaffected.
    QVERIFY(!run->result(2).isSuccess());
    QCOMPARE(run->result(2).error.message(), QStringLiteral("refused"));
    QVERIFY(!run->result(5).isSuccess());
    QVERIFY(run->result(1).isSuccess());
    QCOMPARE(run->result(6).content(), QStringLiteral("prompt 6"));

    // contents() keeps the alignment by leaving a hole rather than closing it.
    const QStringList texts = run->contents();
    QCOMPARE(texts.size(), 7);
    QVERIFY(texts.at(2).isEmpty());
    QCOMPARE(texts.at(3), QStringLiteral("prompt 3"));
}

void TestChatMap::maxFailuresStopsARunThatIsAllFailing()
{
    // A wrong API key fails every item; burning a thousand requests to discover
    // that is a waste worth stopping.
    EchoServer server(10);
    const QStringList all = prompts(20);
    server.setFailing(QSet<QString>(all.cbegin(), all.cend()));

    Client client;
    client.setBaseUrl(server.baseUrl());
    client.setRetryPolicy(RetryPolicy::none());

    ChatMap map(&client);
    map.setConcurrency(2);
    map.setMaxFailures(3);
    QCOMPARE(map.maxFailures(), 3);

    auto *run = map.map(QStringLiteral("m"), prompts(20));
    QVERIFY(settled(run));

    QVERIFY(run->isAborted());
    QCOMPARE(run->successCount(), 0);
    // It stopped early: nothing like all twenty went out.
    QVERIFY2(server.requestCount() < 20,
             qPrintable(QStringLiteral("%1 requests went out").arg(server.requestCount())));
    QVERIFY(run->failureCount() >= 3);
}

void TestChatMap::progressIsReportedPerItem()
{
    EchoServer server(10);
    Client client;
    client.setBaseUrl(server.baseUrl());

    ChatMap map(&client);
    auto *run = map.map(QStringLiteral("m"), prompts(5));

    QList<int> seen;
    connect(run, &ChatMapReply::itemFinished, [&seen](int index, const ChatMapItem &item) {
        // The item is complete by the time it is announced -- a progress signal
        // that arrives before the value it refers to is useless.
        QVERIFY(item.finished);
        QCOMPARE(item.index, index);
        seen.append(index);
    });
    QSignalSpy progress(run, &ChatMapReply::progress);

    QVERIFY(settled(run));
    QCOMPARE(seen.size(), 5);
    QCOMPARE(progress.count(), 5);
    QCOMPARE(progress.last().at(0).toInt(), 5);
    QCOMPARE(progress.last().at(1).toInt(), 5);

    // Every index exactly once, whatever order they finished in.
    std::sort(seen.begin(), seen.end());
    QCOMPARE(seen, QList<int>({0, 1, 2, 3, 4}));
}

void TestChatMap::anEmptyRunFinishesAtOnce()
{
    // And it announces itself *after* being returned, so a caller still gets to
    // connect -- otherwise the one signal it emits would be the one nobody
    // could have heard.
    StubServer server(completion(QStringLiteral("unused")));
    Client client;
    client.setBaseUrl(server.baseUrl());

    ChatMap map(&client);
    auto *run = map.map(QStringLiteral("m"), QStringList());
    QVERIFY(!run->isFinished());

    QSignalSpy finished(run, &ChatMapReply::allFinished);
    QVERIFY(finished.wait(2000));
    QCOMPARE(run->count(), 0);
    QCOMPARE(run->contents(), QStringList());
    QCOMPARE(server.requestCount(), 0);
}

void TestChatMap::abortStopsIssuingAndStillFinishes()
{
    EchoServer server(60);
    Client client;
    client.setBaseUrl(server.baseUrl());

    ChatMap map(&client);
    map.setConcurrency(2);

    auto *run = map.map(QStringLiteral("m"), prompts(12));
    QSignalSpy finished(run, &ChatMapReply::allFinished);

    // Let a couple go out, then give up.
    QTRY_VERIFY_WITH_TIMEOUT(server.requestCount() >= 2, 5000);
    run->abort();
    QVERIFY(run->isAborted());

    // Still finishes: a caller waiting on allFinished() must not be left
    // waiting because it was the one who gave up.
    QVERIFY(run->isFinished() || finished.wait(5000));
    QVERIFY(run->finishedCount() < 12);
    QVERIFY2(server.requestCount() < 12,
             qPrintable(QStringLiteral("%1 requests went out").arg(server.requestCount())));

    // Aborting twice is not two endings.
    run->abort();
    QCOMPARE(finished.count(), 1);
}

void TestChatMap::requestsCanDifferPerItem()
{
    // The prompts overload is a convenience over this one; a run where each
    // item has its own model or parameters has to be expressible.
    EchoServer server(10);
    Client client;
    client.setBaseUrl(server.baseUrl());

    QList<ChatCompletionRequest> requests;
    for (int i = 0; i < 3; ++i) {
        ChatCompletionRequest request(QStringLiteral("model-%1").arg(i),
                                      {Message::user(QStringLiteral("prompt %1").arg(i))});
        request.setTemperature(0.0);
        requests.append(request);
    }

    ChatMap map(&client);
    auto *run = map.map(requests);
    QVERIFY(settled(run));
    QCOMPARE(run->successCount(), 3);
    QCOMPARE(run->contents(), prompts(3));
}

void TestChatMap::aMapWithoutAClientFailsEveryItem()
{
    // Every item reports why rather than the run hanging on requests that were
    // never made.
    ChatMap map(nullptr);
    auto *run = map.map(QStringLiteral("m"), prompts(3));
    QVERIFY(settled(run, 2000));

    QCOMPARE(run->failureCount(), 3);
    QCOMPARE(run->result(0).error.kind(), ClientError::Kind::InvalidRequest);
}

QTEST_MAIN(TestChatMap)
#include "tst_chatmap.moc"
