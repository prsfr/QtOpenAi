// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/MetricsCollector.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

namespace {

QByteArray completionBody(int promptTokens, int completionTokens)
{
    return QStringLiteral(R"({"id":"c","object":"chat.completion","created":1,"model":"gpt-4o-mini",
        "choices":[{"index":0,"message":{"role":"assistant","content":"hi"},
                    "finish_reason":"stop"}],
        "usage":{"prompt_tokens":%1,"completion_tokens":%2,"total_tokens":%3}})")
            .arg(promptTokens)
            .arg(completionTokens)
            .arg(promptTokens + completionTokens)
            .toUtf8();
}

ChatCompletionRequest sampleRequest()
{
    return ChatCompletionRequest(QStringLiteral("gpt-4o-mini"),
                                 {Message::user(QStringLiteral("hi"))});
}

// Streams two fragments with a pause before the first, so time-to-first-token
// is measurably shorter than the whole request.
class SlowStreamServer : public QObject
{
    Q_OBJECT
public:
    explicit SlowStreamServer(QObject *parent = nullptr)
        : QObject(parent)
    {
        m_server.listen(QHostAddress::LocalHost, 0);
        connect(&m_server, &QTcpServer::newConnection, this, &SlowStreamServer::onConnection);
    }

    QUrl baseUrl() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1/v1").arg(m_server.serverPort()));
    }

private slots:
    void onConnection()
    {
        QTcpSocket *socket = m_server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            m_request += socket->readAll();
            if (!m_request.contains("\r\n\r\n"))
                return;

            socket->write("HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/event-stream\r\n"
                          "Connection: close\r\n\r\n");
            socket->flush();

            // The gap before the first fragment is what time-to-first-token
            // measures; the gap after it is not.
            QTimer::singleShot(60, socket, [socket] {
                socket->write("data: {\"id\":\"c\",\"object\":\"chat.completion.chunk\","
                              "\"created\":1,\"model\":\"gpt-4o-mini\",\"choices\":[{\"index\":0,"
                              "\"delta\":{\"content\":\"hi\"}}]}\n\n");
                socket->flush();
                QTimer::singleShot(60, socket, [socket] {
                    socket->write("data: [DONE]\n\n");
                    socket->flush();
                    socket->disconnectFromHost();
                });
            });
        });
    }

private:
    QTcpServer m_server;
    QByteArray m_request;
};

} // namespace

// Coverage for metrics and observability (#36). Everything here runs against
// the offline stubs -- what is being measured is this library's bookkeeping,
// not a provider's.
class TestMetrics : public QObject
{
    Q_OBJECT
private slots:
    void timesEveryRequestAClientMakes();
    void countsFailuresByStatus();
    void aggregatesTokensPerModel();
    void estimatesCostFromTheCatalog();
    void chargesNothingForAModelItHasNoPriceFor();
    void takesACorrectedPriceTable();
    void measuresTimeToFirstTokenForStreams();
    void keepsTheLatestRateLimitHeadroom();
    void stopsRecordingOnceDetached();
    void resetClearsTheCounts();
};

void TestMetrics::timesEveryRequestAClientMakes()
{
    StubServer server(completionBody(10, 5));
    Client client(server.baseUrl(), QStringLiteral("k"));

    MetricsCollector metrics;
    metrics.attach(&client);
    QSignalSpy recordedSpy(&metrics, &MetricsCollector::requestRecorded);

    QVERIFY(awaited(client.createChatCompletion(sampleRequest())));
    QVERIFY(awaited(client.createChatCompletion(sampleRequest())));

    const MetricsSnapshot snapshot = metrics.snapshot();
    QCOMPARE(snapshot.requests, 2);
    QCOMPARE(snapshot.successes, 2);
    QCOMPARE(snapshot.failures, 0);
    QCOMPARE(recordedSpy.count(), 2);
    // Timed, not merely counted -- a request over loopback still takes a
    // measurable moment, and never a negative one.
    QVERIFY(snapshot.averageDurationMs() >= 0);
    QVERIFY(snapshot.slowestDurationMs >= 0);
    // Nothing streamed, so there is no time-to-first-token to report.
    QCOMPARE(snapshot.streamedRequests, 0);
    QCOMPARE(snapshot.averageTimeToFirstTokenMs(), 0.0);
}

void TestMetrics::countsFailuresByStatus()
{
    StubServer server(429, R"({"error":{"message":"slow down","type":"rate_limit_error"}})");
    Client client(server.baseUrl(), QStringLiteral("k"));
    // Retries would turn one failure into several requests; this measures the
    // bookkeeping, not the retry policy.
    client.setRetryPolicy(RetryPolicy::none());

    MetricsCollector metrics;
    metrics.attach(&client);

    const auto reply = awaited(client.createChatCompletion(sampleRequest()));
    QVERIFY(reply);
    QVERIFY(!reply->isSuccess());

    const MetricsSnapshot snapshot = metrics.snapshot();
    QCOMPARE(snapshot.requests, 1);
    QCOMPARE(snapshot.successes, 0);
    QCOMPARE(snapshot.failures, 1);
    QCOMPARE(snapshot.failuresByStatus.value(429), 1);
}

void TestMetrics::aggregatesTokensPerModel()
{
    StubServer server(completionBody(10, 5));
    Client client(server.baseUrl(), QStringLiteral("k"));

    MetricsCollector metrics;
    metrics.attach(&client);
    QSignalSpy usageSpy(&metrics, &MetricsCollector::usageRecorded);

    // Tokens live in the typed response, so the typed reply is what carries
    // them back.
    QVERIFY(awaited(metrics.observe(client.createChatCompletion(sampleRequest()))));
    QVERIFY(awaited(metrics.observe(client.createChatCompletion(sampleRequest()))));

    const ModelMetrics model = metrics.metrics(QStringLiteral("gpt-4o-mini"));
    QCOMPARE(model.requests, 2);
    QCOMPARE(model.promptTokens, 20);
    QCOMPARE(model.completionTokens, 10);
    QCOMPARE(model.totalTokens, 30);
    QCOMPARE(usageSpy.count(), 2);

    // Totals are the sum across models, and a model nobody used has none.
    QCOMPARE(metrics.snapshot().totals().totalTokens, 30);
    QCOMPARE(metrics.metrics(QStringLiteral("gpt-4o")).requests, 0);
}

void TestMetrics::estimatesCostFromTheCatalog()
{
    MetricsCollector metrics;

    Usage usage;
    usage.setPromptTokens(1000000);
    usage.setCompletionTokens(1000000);
    metrics.recordUsage(QStringLiteral("gpt-4o-mini"), usage);

    // A million of each, so the answer is the price list read straight off.
    const ModelInfo info = ModelCatalog::shared().model(QStringLiteral("gpt-4o-mini"));
    QCOMPARE(metrics.metrics(QStringLiteral("gpt-4o-mini")).cost,
             info.inputPrice() + info.outputPrice());
    QCOMPARE(metrics.snapshot().cost(), info.inputPrice() + info.outputPrice());
}

void TestMetrics::chargesNothingForAModelItHasNoPriceFor()
{
    // Guessing a price would be worse than admitting to none; the tokens are
    // still counted.
    MetricsCollector metrics;

    Usage usage;
    usage.setPromptTokens(1000);
    metrics.recordUsage(QStringLiteral("some-local-model"), usage);

    QCOMPARE(metrics.metrics(QStringLiteral("some-local-model")).promptTokens, 1000);
    QCOMPARE(metrics.metrics(QStringLiteral("some-local-model")).cost, 0.0);
}

void TestMetrics::takesACorrectedPriceTable()
{
    // Prices change; the catalog is data, and so is the one a collector uses.
    ModelCatalog catalog;
    ModelInfo info(QStringLiteral("house-model"));
    info.setInputPrice(2.0);
    info.setOutputPrice(4.0);
    catalog.insert(info);

    MetricsCollector metrics;
    metrics.setCatalog(catalog);

    Usage usage;
    usage.setPromptTokens(500000);
    usage.setCompletionTokens(250000);
    metrics.recordUsage(QStringLiteral("house-model"), usage);

    QCOMPARE(metrics.metrics(QStringLiteral("house-model")).cost, 0.5 * 2.0 + 0.25 * 4.0);
}

void TestMetrics::measuresTimeToFirstTokenForStreams()
{
    // What a user perceives as latency is the wait before the first fragment,
    // not the length of the whole stream.
    SlowStreamServer server;
    Client client(server.baseUrl(), QStringLiteral("k"));

    MetricsCollector metrics;
    metrics.attach(&client);

    ChatCompletionStreamReply *reply = client.createChatCompletionStream(sampleRequest());
    QSignalSpy doneSpy(reply, &ChatCompletionStreamReply::done);
    QVERIFY(doneSpy.wait(5000));

    const MetricsSnapshot snapshot = metrics.snapshot();
    QCOMPARE(snapshot.requests, 1);
    QCOMPARE(snapshot.streamedRequests, 1);
    // The stub waits before the first fragment and again before closing, so the
    // two numbers must differ in the right direction.
    QVERIFY(snapshot.averageTimeToFirstTokenMs() > 0);
    QVERIFY(snapshot.averageTimeToFirstTokenMs() < double(snapshot.totalDurationMs));
}

void TestMetrics::keepsTheLatestRateLimitHeadroom()
{
    // Only the newest reading says anything about now, so it replaces rather
    // than accumulating.
    MetricsCollector metrics;

    RequestMetrics first;
    first.ok = true;
    first.rateLimit.remainingRequests = 90;
    metrics.recordRequest(first);

    RequestMetrics second;
    second.ok = true;
    second.rateLimit.remainingRequests = 42;
    metrics.recordRequest(second);

    // A request that reported nothing does not erase what was known.
    RequestMetrics silent;
    silent.ok = true;
    metrics.recordRequest(silent);

    QCOMPARE(metrics.snapshot().rateLimit.remainingRequests, 42);
    QCOMPARE(metrics.snapshot().requests, 3);
}

void TestMetrics::stopsRecordingOnceDetached()
{
    StubServer server(completionBody(1, 1));
    Client client(server.baseUrl(), QStringLiteral("k"));

    MetricsCollector metrics;
    metrics.attach(&client);
    QVERIFY(awaited(client.createChatCompletion(sampleRequest())));
    QCOMPARE(metrics.snapshot().requests, 1);

    metrics.detach(&client);
    QVERIFY(awaited(client.createChatCompletion(sampleRequest())));

    // Detaching stops the recording without discarding what was recorded.
    QCOMPARE(metrics.snapshot().requests, 1);
}

void TestMetrics::resetClearsTheCounts()
{
    MetricsCollector metrics;

    Usage usage;
    usage.setPromptTokens(100);
    metrics.recordUsage(QStringLiteral("gpt-4o-mini"), usage);
    RequestMetrics request;
    request.ok = true;
    metrics.recordRequest(request);

    QCOMPARE(metrics.snapshot().requests, 1);
    metrics.reset();

    QCOMPARE(metrics.snapshot().requests, 0);
    QCOMPARE(metrics.snapshot().models.size(), 0);
    QCOMPARE(metrics.metrics(QStringLiteral("gpt-4o-mini")).promptTokens, 0);
}

QTEST_MAIN(TestMetrics)
#include "tst_metrics.moc"
