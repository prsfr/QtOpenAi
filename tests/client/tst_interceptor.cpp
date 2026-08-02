// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/Interceptor.h>
#include <QtOpenAi/Client/LoggingInterceptor.h>

#include <QtCore/QUrlQuery>
#include <QtTest/QtTest>

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

namespace {

const char kCompletion[] = R"({"id":"c","object":"chat.completion","created":1,
    "model":"m","choices":[{"index":0,"finish_reason":"stop",
    "message":{"role":"assistant","content":"hi"}}]})";

const char kCanned[] = R"({"id":"cached","object":"chat.completion","created":1,
    "model":"m","choices":[{"index":0,"finish_reason":"stop",
    "message":{"role":"assistant","content":"from the cache"}}]})";

ChatCompletionRequest sampleRequest()
{
    return ChatCompletionRequest(QStringLiteral("m"), {Message::user(QStringLiteral("hi"))});
}

// Records which hook ran, in the order it ran, into a list shared by the test.
// The interceptors under test are mostly about *ordering*, and a shared log is
// the only way to observe an order that spans several objects.
class RecordingInterceptor : public Interceptor
{
    Q_OBJECT
public:
    RecordingInterceptor(QString name, QStringList *log, QObject *parent = nullptr)
        : Interceptor(parent)
        , m_name(std::move(name))
        , m_log(log)
    { }

    std::optional<InterceptedResponse> beforeRequest(InterceptedRequest &request) override
    {
        m_log->append(QStringLiteral("%1.before").arg(m_name));
        m_method = request.method;
        m_body = request.body;
        return std::nullopt;
    }

    void afterResponse(const InterceptedResponse &response) override
    {
        m_log->append(QStringLiteral("%1.after").arg(m_name));
        m_seen = response;
    }

    QByteArray m_method;
    QByteArray m_body;
    InterceptedResponse m_seen;

private:
    QString m_name;
    QStringList *m_log;
};

// Stamps a header and a query parameter whose values differ per request -- the
// case Client::setDefaultHeader() cannot serve, and the reason interceptors get
// to modify the request at all.
class TraceInterceptor : public Interceptor
{
    Q_OBJECT
public:
    using Interceptor::Interceptor;

    std::optional<InterceptedResponse> beforeRequest(InterceptedRequest &request) override
    {
        request.request.setRawHeader("X-Trace-Id", "trace-42");

        QUrl url = request.url();
        QUrlQuery query(url);
        query.addQueryItem(QStringLiteral("access_token"), QStringLiteral("sk-in-the-url"));
        url.setQuery(query);
        request.request.setUrl(url);
        return std::nullopt;
    }
};

// Answers every request from a canned body, which is what a cache does.
class AnsweringInterceptor : public Interceptor
{
    Q_OBJECT
public:
    using Interceptor::Interceptor;

    std::optional<InterceptedResponse> beforeRequest(InterceptedRequest &) override
    {
        InterceptedResponse answer;
        answer.body = kCanned;
        return answer;
    }
};

} // namespace

// Coverage for the interceptor chain (#49).
class TestInterceptor : public QObject
{
    Q_OBJECT
private slots:
    void noneIsInstalledByDefault();
    void hooksNestAroundARequest();
    void anInterceptorCanChangeTheRequest();
    void anInterceptorCanAnswerWithoutTheNetwork();
    void afterResponseSeesAFailure();
    void streamsGetTheOutgoingHalf();
    void aRemovedInterceptorIsNotCalled();
    void aDestroyedInterceptorRemovesItself();
    void theLoggerNeverWritesTheApiKey();
    void theLoggerKeepsBodiesOutOfTheLogUnlessAsked();
};

void TestInterceptor::noneIsInstalledByDefault()
{
    // "Nothing is paid for when nothing is installed" starts with nothing being
    // installed -- and a request still has to work.
    Client client;
    QVERIFY(client.interceptors().isEmpty());

    StubServer server(kCompletion);
    client.setBaseUrl(server.baseUrl());
    const auto reply = awaited(client.createChatCompletion(sampleRequest()));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    // Adding the same one twice is one interceptor, not two.
    QStringList log;
    RecordingInterceptor first(QStringLiteral("a"), &log);
    client.addInterceptor(&first);
    client.addInterceptor(&first);
    QCOMPARE(client.interceptors().size(), 1);
}

void TestInterceptor::hooksNestAroundARequest()
{
    QStringList log;
    RecordingInterceptor outer(QStringLiteral("outer"), &log);
    RecordingInterceptor inner(QStringLiteral("inner"), &log);

    StubServer server(kCompletion);
    Client client;
    client.setBaseUrl(server.baseUrl());
    client.addInterceptor(&outer);
    client.addInterceptor(&inner);

    const auto reply = awaited(client.createChatCompletion(sampleRequest()));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    // Out in installation order, back in reverse: the first installed is the
    // outermost, so its afterResponse() wraps everything the others did.
    QCOMPARE(log, QStringList({QStringLiteral("outer.before"), QStringLiteral("inner.before"),
                               QStringLiteral("inner.after"), QStringLiteral("outer.after")}));

    // What each hook was handed.
    QCOMPARE(outer.m_method, QByteArray("POST"));
    QVERIFY(outer.m_body.contains("\"model\":\"m\""));
    QCOMPARE(outer.m_seen.httpStatus, 200);
    QVERIFY(outer.m_seen.body.contains("chat.completion"));
    QVERIFY(!outer.m_seen.fromCache);
    QVERIFY(!outer.m_seen.error.isError());
    QCOMPARE(outer.m_seen.url().host(), QStringLiteral("127.0.0.1"));
    // The exchange carries the request that caused it, so the two hooks can be
    // correlated without the interceptor holding state across them.
    QCOMPARE(outer.m_seen.request.method, QByteArray("POST"));
    QVERIFY(outer.m_seen.request.body.contains("\"model\":\"m\""));
}

void TestInterceptor::anInterceptorCanChangeTheRequest()
{
    TraceInterceptor trace;

    StubServer server(kCompletion);
    Client client;
    client.setBaseUrl(server.baseUrl());
    client.addInterceptor(&trace);

    const auto reply = awaited(client.createChatCompletion(sampleRequest()));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    // The proof is on the wire, not in the object.
    QVERIFY2(server.requestHeaders().toLower().contains("x-trace-id: trace-42"),
             server.requestHeaders().constData());
    QVERIFY2(server.requestLine().contains("access_token=sk-in-the-url"),
             server.requestLine().constData());
}

void TestInterceptor::anInterceptorCanAnswerWithoutTheNetwork()
{
    AnsweringInterceptor cache;
    QStringList log;
    RecordingInterceptor behind(QStringLiteral("behind"), &log);

    StubServer server(kCompletion);
    Client client;
    client.setBaseUrl(server.baseUrl());
    client.addInterceptor(&cache);
    client.addInterceptor(&behind);

    const auto reply = awaited(client.createChatCompletion(sampleRequest()));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    // The canned body was decoded by the ordinary typed reply ...
    QCOMPARE(reply->response().id(), QStringLiteral("cached"));
    QCOMPARE(reply->response().choices().at(0).message().content(),
             QStringLiteral("from the cache"));
    // ... and the server was never touched.
    QCOMPARE(server.requestCount(), 0);

    // An answer stops the outgoing chain but not the returning one: an
    // interceptor behind the cache still gets to see (and log, and count) the
    // exchange, flagged as not having come from the network.
    QCOMPARE(log, QStringList({QStringLiteral("behind.after")}));
    QVERIFY(behind.m_seen.fromCache);
    QCOMPARE(behind.m_seen.httpStatus, 200);
}

void TestInterceptor::afterResponseSeesAFailure()
{
    QStringList log;
    RecordingInterceptor recorder(QStringLiteral("r"), &log);

    StubServer server(500, R"({"error":{"message":"boom","type":"server_error"}})");
    Client client;
    client.setBaseUrl(server.baseUrl());
    client.setRetryPolicy(RetryPolicy::none());
    client.addInterceptor(&recorder);

    const auto reply = awaited(client.createChatCompletion(sampleRequest()));
    QVERIFY(reply);
    QVERIFY(!reply->isSuccess());

    // A failed request is exactly the one worth logging, so the hook has to
    // fire for it -- with the status, the body and the reason.
    QCOMPARE(log, QStringList({QStringLiteral("r.before"), QStringLiteral("r.after")}));
    QCOMPARE(recorder.m_seen.httpStatus, 500);
    QVERIFY(!recorder.m_seen.isSuccess());
    QVERIFY(recorder.m_seen.body.contains("boom"));
    QCOMPARE(recorder.m_seen.error.message(), QStringLiteral("boom"));
}

void TestInterceptor::streamsGetTheOutgoingHalf()
{
    TraceInterceptor trace;

    StubServer server("data: [DONE]\n\n", "text/event-stream");
    Client client;
    client.setBaseUrl(server.baseUrl());
    client.addInterceptor(&trace);

    auto *reply = client.createChatCompletionStream(sampleRequest());
    reply->setAutoDelete(false);
    QSignalSpy done(reply, &ChatCompletionStreamReply::done);
    QVERIFY(done.wait(5000));
    reply->deleteLater();

    // A stream has no single body for afterResponse(), but the request still
    // goes out through the chain -- so a trace header reaches a streamed call
    // exactly as it reaches a one-shot one.
    QVERIFY2(server.requestHeaders().toLower().contains("x-trace-id: trace-42"),
             server.requestHeaders().constData());
}

void TestInterceptor::aRemovedInterceptorIsNotCalled()
{
    QStringList log;
    RecordingInterceptor recorder(QStringLiteral("r"), &log);

    StubServer server(QList<StubServer::Response> {{kCompletion}, {kCompletion}});
    Client client;
    client.setBaseUrl(server.baseUrl());
    client.addInterceptor(&recorder);
    QVERIFY(awaited(client.createChatCompletion(sampleRequest())));
    QCOMPARE(log.size(), 2);

    client.removeInterceptor(&recorder);
    QVERIFY(client.interceptors().isEmpty());
    QVERIFY(awaited(client.createChatCompletion(sampleRequest())));
    QCOMPARE(log.size(), 2);
}

void TestInterceptor::aDestroyedInterceptorRemovesItself()
{
    // The client holds a raw pointer it does not own, so an interceptor that
    // goes away first must not leave one behind -- otherwise the next request
    // calls through a dangling pointer.
    StubServer server(kCompletion);
    Client client;
    client.setBaseUrl(server.baseUrl());

    QStringList log;
    {
        RecordingInterceptor scoped(QStringLiteral("scoped"), &log);
        client.addInterceptor(&scoped);
        QCOMPARE(client.interceptors().size(), 1);
    }
    QVERIFY(client.interceptors().isEmpty());

    const auto reply = awaited(client.createChatCompletion(sampleRequest()));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QVERIFY(log.isEmpty());
}

void TestInterceptor::theLoggerNeverWritesTheApiKey()
{
    LoggingInterceptor logger;
    QStringList lines;
    connect(&logger, &LoggingInterceptor::logged, &logger,
            [&lines](const QString &line) { lines.append(line); });

    TraceInterceptor trace; // puts a secret-looking parameter in the URL

    StubServer server(kCompletion);
    Client client;
    client.setBaseUrl(server.baseUrl());
    client.setApiKey(QStringLiteral("sk-do-not-log-me"));
    client.setOrganization(QStringLiteral("org-secret"));
    client.addInterceptor(&trace);
    client.addInterceptor(&logger); // behind the trace, so it sees the final URL

    const auto reply = awaited(client.createChatCompletion(sampleRequest()));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    const QString written = lines.join(QLatin1Char('\n'));
    QVERIFY2(!written.contains(QStringLiteral("sk-do-not-log-me")), qPrintable(written));
    QVERIFY2(!written.contains(QStringLiteral("sk-in-the-url")), qPrintable(written));
    QVERIFY2(!written.contains(QStringLiteral("org-secret")), qPrintable(written));

    // Redacted, not omitted: knowing an Authorization header was sent is half
    // of what one debugs.
    QVERIFY2(written.contains(QStringLiteral("<redacted>")), qPrintable(written));
    QVERIFY2(written.contains(QStringLiteral("--> POST")), qPrintable(written));
    QVERIFY2(written.contains(QStringLiteral("<-- 200 POST")), qPrintable(written));

    // The list is replaceable, and replacing it really does stop the redaction
    // -- a caller who narrows it is not silently still protected.
    logger.setRedactedHeaders({"x-nothing"});
    QCOMPARE(logger.redactedHeaders(), QList<QByteArray>({"x-nothing"}));
    QVERIFY(LoggingInterceptor::defaultRedactedHeaders().contains("authorization"));
}

void TestInterceptor::theLoggerKeepsBodiesOutOfTheLogUnlessAsked()
{
    LoggingInterceptor logger;
    QStringList lines;
    connect(&logger, &LoggingInterceptor::logged, &logger,
            [&lines](const QString &line) { lines.append(line); });

    StubServer server(QList<StubServer::Response> {{kCompletion}, {kCompletion}});
    Client client;
    client.setBaseUrl(server.baseUrl());
    client.addInterceptor(&logger);

    // Prompts are the user's data, so they stay out of the log by default.
    QVERIFY(!logger.logBodies());
    QVERIFY(awaited(client.createChatCompletion(ChatCompletionRequest(
            QStringLiteral("m"), {Message::user(QStringLiteral("a private prompt"))}))));
    QVERIFY2(!lines.join(QLatin1Char('\n')).contains(QStringLiteral("a private prompt")),
             qPrintable(lines.join(QLatin1Char('\n'))));

    lines.clear();
    logger.setLogBodies(true);
    logger.setMaxBodyLength(20);
    QVERIFY(awaited(client.createChatCompletion(ChatCompletionRequest(
            QStringLiteral("m"), {Message::user(QStringLiteral("a private prompt"))}))));

    const QString written = lines.join(QLatin1Char('\n'));
    // Asked for, and truncated: one large upload must not fill a disk.
    QVERIFY2(written.contains(QStringLiteral("bytes total")), qPrintable(written));
    QVERIFY2(!written.contains(QStringLiteral("a private prompt")), qPrintable(written));
}

QTEST_MAIN(TestInterceptor)
#include "tst_interceptor.moc"
