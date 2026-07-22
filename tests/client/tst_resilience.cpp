// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

namespace {

QByteArray okBody()
{
    return R"({"id":"x","object":"chat.completion","created":1,"model":"gpt-4o",
        "choices":[{"index":0,"message":{"role":"assistant","content":"ok"},
        "finish_reason":"stop"}],"usage":{"prompt_tokens":1,"completion_tokens":1,
        "total_tokens":2}})";
}

} // namespace

// A stub server that serves a scripted sequence of responses across successive
// connections (later connections repeat the last entry) and records each
// request's header block, so retries and outgoing headers can be asserted.
struct StubResponse
{
    int status = 200;
    QByteArray body;
    QList<QPair<QByteArray, QByteArray>> headers;
};

class ScriptedServer : public QObject
{
    Q_OBJECT
public:
    explicit ScriptedServer(QList<StubResponse> script, QObject *parent = nullptr)
        : QObject(parent)
        , m_script(std::move(script))
    {
        m_server.listen(QHostAddress::LocalHost, 0);
        connect(&m_server, &QTcpServer::newConnection, this, &ScriptedServer::onConnection);
    }

    QUrl baseUrl() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1/v1").arg(m_server.serverPort()));
    }

    int connectionCount() const { return m_connectionCount; }
    QList<QByteArray> requests() const { return m_requests; }

private slots:
    void onConnection()
    {
        QTcpSocket *socket = m_server.nextPendingConnection();
        auto buffer = std::make_shared<QByteArray>();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer]() {
            *buffer += socket->readAll();
            if (!buffer->contains("\r\n\r\n"))
                return;
            m_requests.append(*buffer);
            const int index = qMin(m_connectionCount, m_script.size() - 1);
            ++m_connectionCount;
            const StubResponse &r = m_script.at(index);

            QByteArray response = "HTTP/1.1 " + QByteArray::number(r.status)
                                  + " X\r\n"
                                    "Content-Type: application/json\r\n"
                                    "Content-Length: "
                                  + QByteArray::number(r.body.size()) + "\r\n";
            for (const auto &h : r.headers)
                response += h.first + ": " + h.second + "\r\n";
            response += "Connection: close\r\n\r\n" + r.body;
            socket->write(response);
            socket->flush();
            socket->disconnectFromHost();
        });
    }

private:
    QTcpServer m_server;
    QList<StubResponse> m_script;
    int m_connectionCount = 0;
    QList<QByteArray> m_requests;
};

class TestResilience : public QObject
{
    Q_OBJECT
private:
    static RetryPolicy fastPolicy(int maxRetries)
    {
        RetryPolicy p;
        p.maxRetries = maxRetries;
        p.initialDelayMs = 5;
        p.maxDelayMs = 50;
        p.jitter = false;
        return p;
    }

    static ChatCompletionRequest sampleRequest()
    {
        return ChatCompletionRequest(QStringLiteral("gpt-4o"),
                                     {Message::user(QStringLiteral("hi"))});
    }

private slots:
    void retriesOn429ThenSucceeds();
    void retryAfterHeaderOverridesBackoff();
    void exhaustsRetriesThenFails();
    void nonRetryableStatusFailsImmediately();
    void parsesRateLimitHeaders();
    void azureAuthSchemeAndApiVersion();
    void customHeaderAndUserAgent();
};

void TestResilience::retriesOn429ThenSucceeds()
{
    ScriptedServer server({{429, okBody(), {}}, {200, okBody(), {}}});
    Client client(server.baseUrl(), QStringLiteral("k"));
    client.setRetryPolicy(fastPolicy(2));

    ChatCompletionReply *reply = client.createChatCompletion(sampleRequest());
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QCOMPARE(reply->retryCount(), 1);
    QCOMPARE(server.connectionCount(), 2);
    delete reply;
}

void TestResilience::retryAfterHeaderOverridesBackoff()
{
    // Retry-After: 0 → immediate retry, overriding the (large) backoff.
    ScriptedServer server({{429, okBody(), {{"Retry-After", "0"}}}, {200, okBody(), {}}});
    Client client(server.baseUrl(), QStringLiteral("k"));
    RetryPolicy policy = fastPolicy(2);
    policy.initialDelayMs = 5000; // would be slow if not overridden
    client.setRetryPolicy(policy);

    ChatCompletionReply *reply = client.createChatCompletion(sampleRequest());
    reply->setAutoDelete(false);
    QSignalSpy retrySpy(reply, &ChatCompletionReply::retrying);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 3000));

    QVERIFY(reply->isSuccess());
    QCOMPARE(retrySpy.count(), 1);
    QCOMPARE(retrySpy.first().at(1).toInt(), 0); // delay overridden to 0
    delete reply;
}

void TestResilience::exhaustsRetriesThenFails()
{
    ScriptedServer server({{500, R"({"error":{"message":"boom"}})", {}}});
    Client client(server.baseUrl(), QStringLiteral("k"));
    client.setRetryPolicy(fastPolicy(2));

    ChatCompletionReply *reply = client.createChatCompletion(sampleRequest());
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(!reply->isSuccess());
    QCOMPARE(reply->retryCount(), 2);      // 2 retries after the first try
    QCOMPARE(server.connectionCount(), 3); // 1 + 2
    QCOMPARE(reply->error().httpStatus(), 500);
    delete reply;
}

void TestResilience::nonRetryableStatusFailsImmediately()
{
    ScriptedServer server({{400, R"({"error":{"message":"bad"}})", {}}, {200, okBody(), {}}});
    Client client(server.baseUrl(), QStringLiteral("k"));
    client.setRetryPolicy(fastPolicy(3));

    ChatCompletionReply *reply = client.createChatCompletion(sampleRequest());
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(!reply->isSuccess());
    QCOMPARE(reply->retryCount(), 0);
    QCOMPARE(server.connectionCount(), 1);
    delete reply;
}

void TestResilience::parsesRateLimitHeaders()
{
    ScriptedServer server({{200,
                            okBody(),
                            {{"x-ratelimit-limit-requests", "100"},
                             {"x-ratelimit-remaining-requests", "42"},
                             {"x-ratelimit-limit-tokens", "10000"},
                             {"x-ratelimit-remaining-tokens", "9000"},
                             {"x-ratelimit-reset-requests", "1s"}}}});
    Client client(server.baseUrl(), QStringLiteral("k"));

    ChatCompletionReply *reply = client.createChatCompletion(sampleRequest());
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    const RateLimit rl = reply->rateLimit();
    QCOMPARE(rl.limitRequests, 100);
    QCOMPARE(rl.remainingRequests, 42);
    QCOMPARE(rl.limitTokens, 10000);
    QCOMPARE(rl.remainingTokens, 9000);
    QCOMPARE(rl.resetRequestsMs, 1000);
    delete reply;
}

void TestResilience::azureAuthSchemeAndApiVersion()
{
    ScriptedServer server({{200, okBody(), {}}});
    Client client(server.baseUrl(), QStringLiteral("secret"));
    client.setAuthScheme(Client::AuthScheme::AzureApiKey);
    client.setApiVersion(QStringLiteral("2024-06-01"));

    ChatCompletionReply *reply = client.createChatCompletion(sampleRequest());
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QCOMPARE(server.requests().size(), 1);
    // Qt >= 6.10 normalises header-field names to canonical casing (api-key ->
    // Api-Key), so match the header name case-insensitively.
    const QByteArray req = server.requests().first().toLower();
    QVERIFY(req.contains("api-key: secret"));
    QVERIFY(!req.contains("authorization:"));
    QVERIFY(req.contains("api-version=2024-06-01"));
    delete reply;
}

void TestResilience::customHeaderAndUserAgent()
{
    ScriptedServer server({{200, okBody(), {}}});
    Client client(server.baseUrl(), QStringLiteral("k"));
    client.setUserAgent(QStringLiteral("QtOpenAi-Test/1.0"));
    client.setDefaultHeader("X-Custom", "yes");

    ChatCompletionReply *reply = client.createChatCompletion(sampleRequest());
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    const QByteArray req = server.requests().first();
    QVERIFY(req.contains("User-Agent: QtOpenAi-Test/1.0"));
    QVERIFY(req.contains("X-Custom: yes"));
    delete reply;
}

QTEST_MAIN(TestResilience)
#include "tst_resilience.moc"
