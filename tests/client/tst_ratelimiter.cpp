// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/CachingInterceptor.h>
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/RateLimiter.h>

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

const char kCompletion[] = R"({"id":"c","object":"chat.completion","created":1,
    "model":"m","choices":[{"index":0,"finish_reason":"stop",
    "message":{"role":"assistant","content":"hi"}}]})";

ChatCompletionRequest ask(const QString &prompt)
{
    return ChatCompletionRequest(QStringLiteral("m"), {Message::user(prompt)});
}

// A stub that holds each request open for a moment before answering.
//
// The shared StubServer answers immediately, which makes it useless here: no
// two requests ever overlap, so "at most N at once" would pass with any N. This
// one keeps requests in flight long enough for a concurrency cap to be
// something the server can actually observe being violated.
class HoldingServer : public QObject
{
public:
    explicit HoldingServer(int holdMs = 80, QObject *parent = nullptr)
        : QObject(parent)
        , m_holdMs(holdMs)
    {
        m_server.listen(QHostAddress::LocalHost, 0);
        connect(&m_server, &QTcpServer::newConnection, this, &HoldingServer::onConnection);
    }

    QUrl baseUrl() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1/v1").arg(m_server.serverPort()));
    }

    void setStatus(int status) { m_status = status; }
    void setExtraHeaders(const QByteArray &headers) { m_extraHeaders = headers; }

    int requestCount() const { return m_bodies.size(); }
    int peakConcurrency() const { return m_peak; }
    QList<QByteArray> bodies() const { return m_bodies; }

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
            const QByteArray head = buffer->left(headerEnd);
            for (const QByteArray &line : head.split('\n')) {
                const QByteArray trimmed = line.trimmed().toLower();
                if (trimmed.startsWith("content-length:"))
                    contentLength = trimmed.mid(15).trimmed().toInt();
            }
            if (buffer->size() < headerEnd + 4 + contentLength)
                return;

            m_answering.insert(socket);
            m_bodies.append(buffer->mid(headerEnd + 4, contentLength));
            ++m_inFlight;
            m_peak = qMax(m_peak, m_inFlight);

            QTimer::singleShot(m_holdMs, this, [this, socket = QPointer<QTcpSocket>(socket)]() {
                --m_inFlight;
                if (!socket)
                    return;
                const QByteArray body = kCompletion;
                socket->write("HTTP/1.1 " + QByteArray::number(m_status) + " OK\r\n"
                              + "Content-Type: application/json\r\n" + m_extraHeaders
                              + "Content-Length: " + QByteArray::number(body.size())
                              + "\r\nConnection: close\r\n\r\n" + body);
                socket->flush();
                socket->disconnectFromHost();
            });
        });
    }

    QTcpServer m_server;
    QSet<QTcpSocket *> m_answering;
    QList<QByteArray> m_bodies;
    QByteArray m_extraHeaders;
    int m_holdMs;
    int m_status = 200;
    int m_inFlight = 0;
    int m_peak = 0;
};

// Fire `count` requests and wait for all of them to settle.
bool runAll(Client &client, int count, int timeoutMs = 10000)
{
    int done = 0;
    for (int i = 0; i < count; ++i) {
        auto *reply = client.createChatCompletion(ask(QStringLiteral("prompt %1").arg(i)));
        QObject::connect(reply, &ChatCompletionReply::done, [&done]() { ++done; });
    }
    QElapsedTimer elapsed;
    elapsed.start();
    while (done < count && elapsed.elapsed() < timeoutMs)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    return done == count;
}

// Release anything still queued and let it settle before the fixtures go out of
// scope. A reply is not a child of its Client, so one left in flight would
// outlive the network manager it is using -- a test that walks away from a
// queued request crashes the *next* test, not itself.
void drain(Client &client, RateLimiter &limiter)
{
    limiter.flush();
    QTest::qWait(300);
    client.setRateLimiter(nullptr);
}

} // namespace

// Coverage for client-side rate limiting (#43).
class TestRateLimiter : public QObject
{
    Q_OBJECT
private slots:
    void noneIsInstalledByDefault();
    void concurrencyIsCapped();
    void queuedRequestsGoOutInOrder();
    void requestsPerMinuteQueuesTheExcess();
    void tokensPerMinuteQueuesTheExcess();
    void aCacheHitSpendsNoBudget();
    void retryAfterPausesTheWholeClient();
    void flushReleasesWhatIsWaiting();
    void aDestroyedLimiterStrandsNothing();
};

void TestRateLimiter::noneIsInstalledByDefault()
{
    HoldingServer server(10);
    Client client;
    client.setBaseUrl(server.baseUrl());
    QVERIFY(!client.rateLimiter());

    QVERIFY(runAll(client, 4));
    QCOMPARE(server.requestCount(), 4);

    // An installed limiter with every budget at zero limits nothing either --
    // "no limit" has to be expressible, or the object cannot be left attached.
    RateLimiter limiter;
    QCOMPARE(limiter.maxConcurrent(), 0);
    QCOMPARE(limiter.requestsPerMinute(), 0);
    QCOMPARE(limiter.tokensPerMinute(), 0);
    client.setRateLimiter(&limiter);
    QVERIFY(runAll(client, 4));
    QCOMPARE(server.requestCount(), 8);
    QCOMPARE(limiter.queued(), 0);
}

void TestRateLimiter::concurrencyIsCapped()
{
    HoldingServer server(80);
    Client client;
    client.setBaseUrl(server.baseUrl());

    RateLimiter limiter;
    limiter.setMaxConcurrent(2);
    client.setRateLimiter(&limiter);

    QVERIFY(runAll(client, 6));

    // The point of the whole exercise, measured where it matters: at the
    // server, not in the limiter's own bookkeeping.
    QVERIFY2(server.peakConcurrency() <= 2,
             qPrintable(QStringLiteral("peak was %1").arg(server.peakConcurrency())));
    // And every one of them still went out -- a limiter that drops requests
    // would also pass the check above.
    QCOMPARE(server.requestCount(), 6);
    QCOMPARE(limiter.queued(), 0);
    QCOMPARE(limiter.inFlight(), 0);
}

void TestRateLimiter::queuedRequestsGoOutInOrder()
{
    HoldingServer server(30);
    Client client;
    client.setBaseUrl(server.baseUrl());

    RateLimiter limiter;
    limiter.setMaxConcurrent(1);
    client.setRateLimiter(&limiter);

    QVERIFY(runAll(client, 5));
    QCOMPARE(server.requestCount(), 5);
    QCOMPARE(server.peakConcurrency(), 1);

    // FIFO: a queue that reorders would starve whatever is unlucky.
    const QList<QByteArray> bodies = server.bodies();
    for (int i = 0; i < bodies.size(); ++i) {
        const QByteArray expected = QStringLiteral("prompt %1").arg(i).toUtf8();
        QVERIFY2(bodies.at(i).contains(expected), bodies.at(i).constData());
    }
}

void TestRateLimiter::requestsPerMinuteQueuesTheExcess()
{
    HoldingServer server(10);
    Client client;
    client.setBaseUrl(server.baseUrl());

    RateLimiter limiter;
    limiter.setRequestsPerMinute(2);
    client.setRateLimiter(&limiter);

    QSignalSpy queueChanged(&limiter, &RateLimiter::queueChanged);

    for (int i = 0; i < 5; ++i)
        client.createChatCompletion(ask(QStringLiteral("prompt %1").arg(i)));

    // A rolling window, so the excess waits for the window to move rather than
    // for a minute boundary to tick over. Nothing here waits that long: what is
    // being asserted is that the budget was enforced, not how long it lasts.
    QTRY_COMPARE_WITH_TIMEOUT(server.requestCount(), 2, 5000);
    QCOMPARE(limiter.queued(), 3);
    QVERIFY(!queueChanged.isEmpty());

    QTest::qWait(200);
    QCOMPARE(server.requestCount(), 2);

    drain(client, limiter);
}

void TestRateLimiter::tokensPerMinuteQueuesTheExcess()
{
    HoldingServer server(10);
    Client client;
    client.setBaseUrl(server.baseUrl());

    RateLimiter limiter;
    // Small enough that one request's estimate exhausts it. The estimate runs
    // over the serialised body and is deliberately generous.
    limiter.setTokensPerMinute(20);
    client.setRateLimiter(&limiter);

    for (int i = 0; i < 3; ++i)
        client.createChatCompletion(ask(QStringLiteral("prompt %1").arg(i)));

    QTRY_COMPARE_WITH_TIMEOUT(server.requestCount(), 1, 5000);
    QCOMPARE(limiter.queued(), 2);

    drain(client, limiter);
}

void TestRateLimiter::aCacheHitSpendsNoBudget()
{
    // A cached answer makes no request, so charging it against a request budget
    // would be charging for something that did not happen.
    StubServer server(kCompletion);
    Client client;
    client.setBaseUrl(server.baseUrl());

    CachingInterceptor cache;
    client.addInterceptor(&cache);

    RateLimiter limiter;
    limiter.setRequestsPerMinute(1);
    client.setRateLimiter(&limiter);

    QVERIFY(runAll(client, 1));
    QCOMPARE(server.requestCount(), 1);

    // The budget is now spent, yet the identical request still completes at
    // once -- it never reaches the limiter.
    int done = 0;
    auto *reply = client.createChatCompletion(ask(QStringLiteral("prompt 0")));
    connect(reply, &ChatCompletionReply::done, [&done]() { ++done; });
    QTRY_COMPARE_WITH_TIMEOUT(done, 1, 2000);
    QCOMPARE(server.requestCount(), 1);
    QCOMPARE(limiter.queued(), 0);
}

void TestRateLimiter::retryAfterPausesTheWholeClient()
{
    // A 429 says *you* are going too fast, not that this one request was
    // unlucky, so the whole client backs off rather than only the reply that
    // heard it.
    HoldingServer server(5);
    server.setStatus(429);
    server.setExtraHeaders("Retry-After: 30\r\n");

    Client client;
    client.setBaseUrl(server.baseUrl());
    client.setRetryPolicy(RetryPolicy::none());

    RateLimiter limiter;
    QSignalSpy paused(&limiter, &RateLimiter::pausedFor);
    client.setRateLimiter(&limiter);

    QVERIFY(runAll(client, 1));
    QVERIFY(limiter.isPaused());
    QCOMPARE(paused.count(), 1);
    QCOMPARE(paused.first().at(0).toInt(), 30000);

    // Everything issued while the pause is in force waits it out.
    client.createChatCompletion(ask(QStringLiteral("during the pause")));
    QTest::qWait(200);
    QCOMPARE(server.requestCount(), 1);
    QCOMPARE(limiter.queued(), 1);

    // A shorter Retry-After does not shorten a pause already in force.
    limiter.pauseFor(10);
    QVERIFY(limiter.isPaused());

    drain(client, limiter);
}

void TestRateLimiter::flushReleasesWhatIsWaiting()
{
    HoldingServer server(10);
    Client client;
    client.setBaseUrl(server.baseUrl());

    RateLimiter limiter;
    limiter.setRequestsPerMinute(1);
    client.setRateLimiter(&limiter);

    QVERIFY(runAll(client, 1));
    for (int i = 0; i < 3; ++i)
        client.createChatCompletion(ask(QStringLiteral("waiting %1").arg(i)));
    QTRY_COMPARE_WITH_TIMEOUT(limiter.queued(), 3, 2000);

    // Released, not abandoned: a caller holding a reply that would never start
    // waits forever, which is worse than one burst over budget.
    limiter.flush();
    QCOMPARE(limiter.queued(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(server.requestCount(), 4, 5000);
    QTest::qWait(200);
}

void TestRateLimiter::aDestroyedLimiterStrandsNothing()
{
    HoldingServer server(10);
    Client client;
    client.setBaseUrl(server.baseUrl());

    {
        RateLimiter limiter;
        limiter.setRequestsPerMinute(1);
        client.setRateLimiter(&limiter);

        QVERIFY(runAll(client, 1));
        for (int i = 0; i < 2; ++i)
            client.createChatCompletion(ask(QStringLiteral("waiting %1").arg(i)));
        QTRY_COMPARE_WITH_TIMEOUT(limiter.queued(), 2, 2000);
    }
    // The limiter is gone; the requests it was holding are not.
    QVERIFY(!client.rateLimiter());
    QTRY_COMPARE_WITH_TIMEOUT(server.requestCount(), 3, 5000);
    QTest::qWait(200);
}

QTEST_MAIN(TestRateLimiter)
#include "tst_ratelimiter.moc"
