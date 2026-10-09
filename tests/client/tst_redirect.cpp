// SPDX-License-Identifier: MIT
#include <QtOpenAi/Admin/Organization.h>
#include <QtOpenAi/Client/CachingInterceptor.h>
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/Interceptor.h>

#include <QtNetwork/QNetworkAccessManager>
#include <QtTest/QtTest>

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QPointer>

#include <memory>
#include <optional>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;
using namespace QtOpenAi::Admin;

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

// A credential must not leave for an origin the application never configured
// (#203). Qt's default redirect policy follows a 3xx to any host as long as the
// scheme does not downgrade, and copies the request -- headers, key and all --
// so every credential-bearing request now follows redirects only within its own
// origin, and a cross-origin one fails as Kind::Redirect without a retry.
//
// "Another origin" is exercised twice: the same port under another host name
// (127.0.0.1 -> localhost, which is the origin server itself, so the evidence
// is that it is asked only once), and another port on the same host (a second
// server that must never be reached).
class TestRedirect : public QObject
{
    Q_OBJECT
private slots:
    void crossOriginRedirectIsRefused_data();
    void crossOriginRedirectIsRefused();
    void refusedRedirectIsNotRetried();
    void organizationAdminKeyIsNotRedirected();
    void sameOriginRedirectIsFollowed();

    // Stage 6 (hardening) probes.
    void everyRedirectStatusIsRefused_data();
    void everyRedirectStatusIsRefused();
    void locationVariants_data();
    void locationVariants();
    void jsonBodyOnRefusedRedirectStaysRedirect_data();
    void jsonBodyOnRefusedRedirectStaysRedirect();
    void sameOriginHopThenCrossOriginIsRefused_data();
    void sameOriginHopThenCrossOriginIsRefused();
    void sameOriginLoopStaysNetwork();
    void retryThenCrossOriginRedirectIsRefused();
    void interceptorThatIgnoresPolicyKeepsIt();
    void cachingInterceptorDoesNotStoreRefusedRedirect();
    void abortOrDeleteWhileRedirectInFlight_data();
    void abortOrDeleteWhileRedirectInFlight();
    void keyNeverLoggedDuringRefusedRedirect_data();
    void keyNeverLoggedDuringRefusedRedirect();
    void nonHttpLocationFailsAsNetworkError();
    void managerGoneDuringRetryBackoff_data();
    void managerGoneDuringRetryBackoff();
    void clientGoneBeforeFirstAttempt_data();
    void clientGoneBeforeFirstAttempt();
    void noReplyAccumulatesUnderTheManager();
};

namespace {

const QString key = QStringLiteral("sk-redirect-secret");

QByteArray chatBody()
{
    return R"({"id":"c1","object":"chat.completion","model":"gpt-4o",
        "choices":[{"index":0,"message":{"role":"assistant","content":"hi"},
        "finish_reason":"stop"}]})";
}

ChatCompletionRequest chatRequest()
{
    return ChatCompletionRequest(QStringLiteral("gpt-4o"), {Message::user(QStringLiteral("hi"))});
}

// The origin server and, for the other-port case, the server it redirects to.
// The origin answers every request with a 302 to the target's address.
struct Servers
{
    std::unique_ptr<StubServer> origin;
    std::unique_ptr<StubServer> target; // null when the target is the origin itself

    // Requests that reached the target -- for the other-host case the origin's
    // own second request, since `localhost` is where it listens too.
    int targetRequests() const
    {
        return target ? target->requestCount() : origin->requestCount() - 1;
    }
};

Servers makeServers(bool otherHost)
{
    Servers servers;
    servers.origin = std::make_unique<StubServer>(chatBody());
    QByteArray location;
    if (otherHost) {
        location = "http://localhost:" + QByteArray::number(servers.origin->baseUrl().port())
                   + "/v1/x";
    } else {
        servers.target = std::make_unique<StubServer>(chatBody());
        location = "http://127.0.0.1:" + QByteArray::number(servers.target->baseUrl().port())
                   + "/v1/x";
    }
    servers.origin->setResponses(
            {{QByteArray(), 302, "application/json", {{"Location", location}}}});
    return servers;
}

// Issue one call on `client` along the named request path and wait for it to
// settle; returns its error, or nothing if it never settled.
std::optional<ClientError> issue(Client &client, const QString &path)
{
    const auto settle = [](auto *raw) -> std::optional<ClientError> {
        const auto reply = awaited(raw);
        if (!reply)
            return std::nullopt;
        return reply->isSuccess() ? ClientError() : reply->error();
    };
    if (path == QLatin1String("stream"))
        return settle(client.createChatCompletionStream(chatRequest()));
    if (path == QLatin1String("get"))
        return settle(client.getResponse(QStringLiteral("resp_1")));
    if (path == QLatin1String("post"))
        return settle(client.createChatCompletion(chatRequest()));
    if (path == QLatin1String("delete"))
        return settle(client.deleteResponse(QStringLiteral("resp_1")));
    TranscriptionRequest request(QByteArray("RIFFfake"), QStringLiteral("clip.wav"),
                                 QStringLiteral("whisper-1"));
    return settle(client.createTranscription(request));
}

} // namespace

void TestRedirect::crossOriginRedirectIsRefused_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<Client::AuthScheme>("scheme");
    QTest::addColumn<bool>("otherHost");

    const QStringList paths {QStringLiteral("get"), QStringLiteral("post"),
                             QStringLiteral("delete"), QStringLiteral("multipart"),
                             QStringLiteral("stream")};
    for (const QString &path : paths) {
        for (const auto scheme :
             {Client::AuthScheme::BearerToken, Client::AuthScheme::AzureApiKey}) {
            const char *schemeName = scheme == Client::AuthScheme::BearerToken ? "bearer" : "azure";
            QTest::addRow("%s/%s/other-host", qPrintable(path), schemeName)
                    << path << scheme << true;
            QTest::addRow("%s/%s/other-port", qPrintable(path), schemeName)
                    << path << scheme << false;
        }
    }
}

void TestRedirect::crossOriginRedirectIsRefused()
{
    QFETCH(QString, path);
    QFETCH(Client::AuthScheme, scheme);
    QFETCH(bool, otherHost);

    const Servers servers = makeServers(otherHost);
    Client client(servers.origin->baseUrl(), key);
    client.setAuthScheme(scheme);

    const std::optional<ClientError> error = issue(client, path);
    QVERIFY2(error, "the request never settled");

    QCOMPARE(servers.targetRequests(), 0);
    QCOMPARE(servers.origin->requestCount(), 1);
    QCOMPARE(error->kind(), ClientError::Kind::Redirect);
    QVERIFY2(!error->message().contains(key), qPrintable(error->message()));
}

// A refused redirect is an answer, not a transport hiccup: sending the request
// again would only be refused again.
void TestRedirect::refusedRedirectIsNotRetried()
{
    const Servers servers = makeServers(false);
    Client client(servers.origin->baseUrl(), key);
    RetryPolicy policy;
    policy.initialDelayMs = 1;
    client.setRetryPolicy(policy);
    QVERIFY(policy.retryOnNetworkError && policy.maxRetries > 0);

    ChatCompletionReply *reply = client.createChatCompletion(chatRequest());
    reply->setAutoDelete(false);
    const std::unique_ptr<ChatCompletionReply> owner(reply);
    QSignalSpy retrying(reply, &ChatCompletionReply::retrying);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QCOMPARE(retrying.count(), 0);
    QCOMPARE(servers.origin->requestCount(), 1);
    QCOMPARE(servers.target->requestCount(), 0);
    QCOMPARE(reply->error().kind(), ClientError::Kind::Redirect);
}

// Organization sends the admin key through the same request builder, so it is
// covered by the same policy -- and must stay so.
void TestRedirect::organizationAdminKeyIsNotRedirected()
{
    const Servers servers = makeServers(false);
    Organization organization(servers.origin->baseUrl(), QStringLiteral("sk-admin-secret"));

    const auto reply = awaited(organization.listProjects());
    QVERIFY(reply);

    QCOMPARE(servers.target->requestCount(), 0);
    QCOMPARE(servers.origin->requestCount(), 1);
    QVERIFY(!reply->isSuccess());
    QCOMPARE(reply->error().kind(), ClientError::Kind::Redirect);
    QVERIFY(!reply->error().message().contains(QStringLiteral("sk-admin-secret")));
}

// Only crossing origins is refused: a redirect within the server still works.
void TestRedirect::sameOriginRedirectIsFollowed()
{
    StubServer server(
            {{QByteArray(), 302, "application/json", {{"Location", "/v1/second"}}}, {chatBody()}});
    Client client(server.baseUrl(), key);

    const auto reply = awaited(client.createChatCompletion(chatRequest()));
    QVERIFY(reply);

    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(server.requestCount(), 2);
    QVERIFY(server.requestLines().at(1).contains("/v1/second"));
    QCOMPARE(reply->response().id(), QStringLiteral("c1"));
}

// ---------------------------------------------------------------------------
// Stage 6 (hardening) probes for #203. Each one attacks the redirect policy
// from a direction the acceptance tests above do not.

namespace {

QByteArray portOf(const StubServer &server) { return QByteArray::number(server.baseUrl().port()); }

StubServer::Response redirectTo(const QByteArray &location, int status = 302, QByteArray body = {})
{
    return {std::move(body), status, "application/json", {{"Location", location}}};
}

// Records what the request looked like when it reached the interceptor chain,
// and stamps a header the way a tracing interceptor would -- without touching
// the redirect attribute.
class TracingInterceptor : public Interceptor
{
public:
    QVariant policySeen;
    std::optional<InterceptedResponse> beforeRequest(InterceptedRequest &request) override
    {
        policySeen = request.request.attribute(QNetworkRequest::RedirectPolicyAttribute);
        request.request.setRawHeader("X-Trace", "t-1");
        return std::nullopt;
    }
    std::optional<InterceptedResponse> seen;
    void afterResponse(const InterceptedResponse &response) override { seen = response; }
};

// Every message Qt or the library prints while installed, whatever the category.
struct CapturedMessages
{
    static QMutex &mutex()
    {
        static QMutex m;
        return m;
    }
    static QStringList &lines()
    {
        static QStringList l;
        return l;
    }
    static void handler(QtMsgType, const QMessageLogContext &context, const QString &message)
    {
        QMutexLocker lock(&mutex());
        lines() << QStringLiteral("[%1] %2").arg(
                QString::fromUtf8(context.category ? context.category : ""), message);
    }

    CapturedMessages()
    {
        lines().clear();
        QLoggingCategory::setFilterRules(QStringLiteral("*.debug=true"));
        previous = qInstallMessageHandler(&CapturedMessages::handler);
    }
    ~CapturedMessages()
    {
        qInstallMessageHandler(previous);
        QLoggingCategory::setFilterRules(QString());
    }
    QString all() const
    {
        QMutexLocker lock(&mutex());
        return lines().join(QLatin1Char('\n'));
    }
    QtMessageHandler previous = nullptr;
};

void drainEvents()
{
    for (int i = 0; i < 20; ++i) {
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
}

} // namespace

// Qt follows 301, 302, 303, 307 and 308 alike; every one of them must be held
// to the origin, including 307/308, which re-send the body as well.
void TestRedirect::everyRedirectStatusIsRefused_data()
{
    QTest::addColumn<int>("status");
    QTest::addColumn<QString>("path");
    for (const int status : {301, 303, 307, 308}) {
        for (const char *path : {"post", "get", "multipart", "stream"})
            QTest::addRow("%d/%s", status, path) << status << QString::fromLatin1(path);
    }
}

void TestRedirect::everyRedirectStatusIsRefused()
{
    QFETCH(int, status);
    QFETCH(QString, path);

    StubServer target(chatBody());
    StubServer origin({redirectTo("http://127.0.0.1:" + portOf(target) + "/v1/x", status)});
    Client client(origin.baseUrl(), key);

    const std::optional<ClientError> error = issue(client, path);
    QVERIFY2(error, "the request never settled");
    QCOMPARE(target.requestCount(), 0);
    QCOMPARE(origin.requestCount(), 1);
    QCOMPARE(error->kind(), ClientError::Kind::Redirect);
}

// Location spellings: the policy is decided on the resolved URL, so a relative,
// protocol-relative, userinfo-carrying or differently-cased Location must land
// on the same side of the line as its absolute equivalent.
void TestRedirect::locationVariants_data()
{
    QTest::addColumn<QString>("kind");
    QTest::addColumn<bool>("followed");

    QTest::newRow("protocol-relative other host") << "pr-host" << false;
    QTest::newRow("protocol-relative other port") << "pr-port" << false;
    QTest::newRow("userinfo other port") << "userinfo-port" << false;
    QTest::newRow("userinfo other host") << "userinfo-host" << false;
    QTest::newRow("https same host and port") << "https" << false;
    QTest::newRow("absolute same origin, explicit port") << "absolute" << true;
    QTest::newRow("upper-case scheme, same origin") << "upper" << true;
    QTest::newRow("dot-dot relative") << "dotdot" << true;
    QTest::newRow("userinfo same origin") << "userinfo-same" << true;
}

void TestRedirect::locationVariants()
{
    QFETCH(QString, kind);
    QFETCH(bool, followed);

    StubServer target(chatBody());
    StubServer origin(chatBody());
    const QByteArray o = portOf(origin);
    const QByteArray t = portOf(target);
    QByteArray location;
    if (kind == "pr-host")
        location = "//localhost:" + o + "/v1/x";
    else if (kind == "pr-port")
        location = "//127.0.0.1:" + t + "/v1/x";
    else if (kind == "userinfo-port")
        location = "http://user:pw@127.0.0.1:" + t + "/v1/x";
    else if (kind == "userinfo-host")
        location = "http://user@localhost:" + o + "/v1/x";
    else if (kind == "https")
        location = "https://127.0.0.1:" + o + "/v1/x";
    else if (kind == "file")
        location = "file:///etc/hostname";
    else if (kind == "absolute")
        location = "http://127.0.0.1:" + o + "/v1/second";
    else if (kind == "upper")
        location = "HTTP://127.0.0.1:" + o + "/v1/second";
    else if (kind == "dotdot")
        location = "../v1/second";
    else if (kind == "userinfo-same")
        location = "http://user:pw@127.0.0.1:" + o + "/v1/second";
    origin.setResponses({redirectTo(location), {chatBody()}});

    Client client(origin.baseUrl(), key);
    const std::optional<ClientError> error = issue(client, QStringLiteral("post"));
    QVERIFY2(error, "the request never settled");
    QCOMPARE(target.requestCount(), 0);

    if (followed) {
        QCOMPARE(error->kind(), ClientError::Kind::NoError);
        QCOMPARE(origin.requestCount(), 2);
    } else {
        QCOMPARE(error->kind(), ClientError::Kind::Redirect);
        QCOMPARE(origin.requestCount(), 1);
    }
}

// The API's `error` object makes any response an HTTP error -- except a refused
// redirect, which must stay Kind::Redirect whatever its body says (impact K6).
void TestRedirect::jsonBodyOnRefusedRedirectStaysRedirect_data()
{
    QTest::addColumn<QString>("path");
    for (const char *path : {"post", "get", "stream"})
        QTest::newRow(path) << QString::fromLatin1(path);
}

void TestRedirect::jsonBodyOnRefusedRedirectStaysRedirect()
{
    QFETCH(QString, path);

    StubServer target(chatBody());
    StubServer origin(
            {redirectTo("http://127.0.0.1:" + portOf(target) + "/v1/x", 302,
                        R"({"error":{"message":"from the 302","type":"t","code":"c"}})")});
    Client client(origin.baseUrl(), key);

    const std::optional<ClientError> error = issue(client, path);
    QVERIFY2(error, "the request never settled");
    QCOMPARE(target.requestCount(), 0);
    QCOMPARE(error->kind(), ClientError::Kind::Redirect);
    QVERIFY(!error->message().contains(QStringLiteral("from the 302")));
}

// A same-origin hop is followed; the policy must still hold on the hop after it.
void TestRedirect::sameOriginHopThenCrossOriginIsRefused_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<Client::AuthScheme>("scheme");
    for (const char *path : {"post", "stream"}) {
        QTest::addRow("%s/bearer", path)
                << QString::fromLatin1(path) << Client::AuthScheme::BearerToken;
        QTest::addRow("%s/azure", path)
                << QString::fromLatin1(path) << Client::AuthScheme::AzureApiKey;
    }
}

void TestRedirect::sameOriginHopThenCrossOriginIsRefused()
{
    QFETCH(QString, path);
    QFETCH(Client::AuthScheme, scheme);

    StubServer target(chatBody());
    StubServer origin(
            {redirectTo("/v1/second"), redirectTo("http://127.0.0.1:" + portOf(target) + "/v1/x")});
    Client client(origin.baseUrl(), key);
    client.setAuthScheme(scheme);

    const std::optional<ClientError> error = issue(client, path);
    QVERIFY2(error, "the request never settled");
    QCOMPARE(target.requestCount(), 0);
    QCOMPARE(origin.requestCount(), 2);
    QCOMPARE(error->kind(), ClientError::Kind::Redirect);
}

// Unchanged behaviour, pinned so a change to it is a decision: a same-origin
// loop ends in Qt's TooManyRedirectsError, which is still a Network error.
void TestRedirect::sameOriginLoopStaysNetwork()
{
    StubServer origin({redirectTo("/v1/loop")});
    Client client(origin.baseUrl(), key);
    client.setRetryPolicy(RetryPolicy::none());

    const std::optional<ClientError> error = issue(client, QStringLiteral("post"));
    QVERIFY2(error, "the request never settled");
    QCOMPARE(error->kind(), ClientError::Kind::Network);
    QVERIFY(origin.requestCount() > 1);
}

// A retry re-issues the same request; the policy must ride along on it.
void TestRedirect::retryThenCrossOriginRedirectIsRefused()
{
    StubServer target(chatBody());
    StubServer origin({{R"({"error":{"message":"busy"}})", 503, "application/json"},
                       redirectTo("http://127.0.0.1:" + portOf(target) + "/v1/x")});
    Client client(origin.baseUrl(), key);
    RetryPolicy policy;
    policy.initialDelayMs = 1;
    policy.jitter = false;
    client.setRetryPolicy(policy);

    ChatCompletionReply *reply = client.createChatCompletion(chatRequest());
    reply->setAutoDelete(false);
    const std::unique_ptr<ChatCompletionReply> owner(reply);
    QSignalSpy retrying(reply, &ChatCompletionReply::retrying);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QCOMPARE(retrying.count(), 1);
    QCOMPARE(origin.requestCount(), 2);
    QCOMPARE(target.requestCount(), 0);
    QCOMPARE(reply->error().kind(), ClientError::Kind::Redirect);
}

// An interceptor that edits the request but not its redirect attribute must
// leave the policy in force (an interceptor that overwrites it is a non-goal).
void TestRedirect::interceptorThatIgnoresPolicyKeepsIt()
{
    StubServer target(chatBody());
    StubServer origin({redirectTo("http://127.0.0.1:" + portOf(target) + "/v1/x")});
    Client client(origin.baseUrl(), key);
    TracingInterceptor tracing;
    client.addInterceptor(&tracing);

    for (const char *path : {"post", "stream", "multipart"}) {
        const std::optional<ClientError> error = issue(client, QString::fromLatin1(path));
        QVERIFY2(error, path);
        QCOMPARE(error->kind(), ClientError::Kind::Redirect);
        QCOMPARE(tracing.policySeen.toInt(), int(QNetworkRequest::SameOriginRedirectPolicy));
    }
    QCOMPARE(target.requestCount(), 0);
    QCOMPARE(origin.requestCount(), 3);
    QVERIFY(origin.requestHeaders().toLower().contains("x-trace: t-1"));

    // What afterResponse() observers (logging, metrics, the cache) are told.
    // InterceptedResponse::isSuccess() is "200..399", so it reads a refused
    // redirect as a success; the error is what says otherwise.
    QVERIFY(tracing.seen);
    QCOMPARE(tracing.seen->httpStatus, 302);
    QCOMPARE(tracing.seen->error.kind(), ClientError::Kind::Redirect);
    QVERIFY(tracing.seen->isSuccess());
}

// InterceptedResponse::isSuccess() is "200..399", and a refused redirect
// reports 302. The cache must still not store it -- otherwise the next
// identical call is answered "200" with whatever the 302 carried.
void TestRedirect::cachingInterceptorDoesNotStoreRefusedRedirect()
{
    StubServer target(chatBody());
    StubServer origin(
            {redirectTo("http://127.0.0.1:" + portOf(target) + "/v1/x", 302, chatBody())});
    Client client(origin.baseUrl(), key);
    CachingInterceptor cache;
    client.addInterceptor(&cache);
    QSignalSpy stored(&cache, &CachingInterceptor::stored);
    QSignalSpy hit(&cache, &CachingInterceptor::hit);

    for (int i = 0; i < 2; ++i) {
        ChatCompletionReply *reply = client.createChatCompletion(chatRequest());
        reply->setAutoDelete(false);
        const std::unique_ptr<ChatCompletionReply> owner(reply);
        QSignalSpy received(reply, &ChatCompletionReply::responseReceived);
        QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));
        QCOMPARE(reply->error().kind(), ClientError::Kind::Redirect);
        QCOMPARE(received.count(), 1);
        QCOMPARE(received.at(0).at(1).toInt(), 302);
    }
    QCOMPARE(stored.count(), 0);
    QCOMPARE(hit.count(), 0);
    QCOMPARE(origin.requestCount(), 2);
    QCOMPARE(target.requestCount(), 0);
}

// Lifetimes around the refusal: the 302 has been written by the origin and not
// yet read by the client when the reply is aborted or deleted, or the client is
// deleted -- and, last, the client is deleted from inside the reply's failed().
void TestRedirect::abortOrDeleteWhileRedirectInFlight_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("action");
    QTest::addColumn<bool>("redirect");
    for (const bool redirect : {true, false}) {
        for (const char *path : {"post", "stream"}) {
            for (const char *action : {"abort", "delete-reply", "delete-client",
                                       "delete-injected-manager", "delete-client-keep-manager",
                                       "delete-client-in-failed", "delete-reply-in-failed"}) {
                // Without a redirect nothing fails, so there is no failed() to act in.
                if (!redirect && QByteArray(action).endsWith("in-failed"))
                    continue;
                QTest::addRow("%s/%s/%s", redirect ? "302" : "200-control", path, action)
                        << QString::fromLatin1(path) << QString::fromLatin1(action) << redirect;
            }
        }
    }
}

void TestRedirect::abortOrDeleteWhileRedirectInFlight()
{
    QFETCH(QString, path);
    QFETCH(QString, action);
    QFETCH(bool, redirect);

    StubServer target(chatBody());
    StubServer origin(chatBody());
    if (redirect)
        origin.setResponses({redirectTo("http://127.0.0.1:" + portOf(target) + "/v1/x")});
    auto client = std::make_unique<Client>(origin.baseUrl(), key);
    // abort() of a REST reply is itself retried under the default policy
    // (pre-existing, unrelated to #203); keep each case to one attempt.
    client->setRetryPolicy(RetryPolicy::none());
    auto manager = std::make_unique<QNetworkAccessManager>();
    if (action == "delete-injected-manager" || action == "delete-client-keep-manager")
        client->setNetworkAccessManager(manager.get());

    QPointer<QObject> reply;
    std::function<void()> abort;
    std::function<bool()> finished;
    std::function<ClientError()> error;
    if (path == "stream") {
        ChatCompletionStreamReply *r = client->createChatCompletionStream(chatRequest());
        r->setAutoDelete(false);
        reply = r;
        abort = [r] { r->abort(); };
        finished = [r] { return r->isFinished(); };
        error = [r] { return r->error(); };
        QObject::connect(r, &ChatCompletionStreamReply::failed, r, [&]() {
            if (action == "delete-client-in-failed")
                client.reset();
            else if (action == "delete-reply-in-failed")
                reply->deleteLater();
        });
    } else {
        ChatCompletionReply *r = client->createChatCompletion(chatRequest());
        r->setAutoDelete(false);
        reply = r;
        abort = [r] { r->abort(); };
        finished = [r] { return r->isFinished(); };
        error = [r] { return r->error(); };
        QObject::connect(r, &ChatCompletionReply::failed, r, [&]() {
            if (action == "delete-client-in-failed")
                client.reset();
            else if (action == "delete-reply-in-failed")
                reply->deleteLater();
        });
    }

    QSignalSpy done(reply.data(), SIGNAL(done()));

    if (action.endsWith("in-failed")) {
        QVERIFY(QTest::qWaitFor([&] { return !client || !reply; }, 5000));
    } else {
        QVERIFY(QTest::qWaitFor([&] { return origin.requestCount() == 1; }, 5000));
        const bool settledBefore = finished();
        if (action == "abort") {
            abort();
            QVERIFY(QTest::qWaitFor([&] { return finished(); }, 5000));
            QVERIFY(error().kind() != ClientError::Kind::NoError);
        } else if (action == "delete-reply") {
            delete reply.data();
        } else {
            // The manager going away -- with the Client or on its own -- ends a
            // request still in flight as a Network failure, settled once and
            // never from inside the deletion; with the manager kept alive the
            // request runs to its own end.
            if (action == "delete-injected-manager")
                manager.reset();
            else
                client.reset();
            QVERIFY(!settledBefore || done.count() == 1);
            QVERIFY(QTest::qWaitFor([&] { return finished(); }, 5000));
            QCOMPARE(done.count(), 1);
            if (action == "delete-client-keep-manager") {
                QCOMPARE(error().kind(),
                         redirect ? ClientError::Kind::Redirect : ClientError::Kind::NoError);
            } else if (!settledBefore) {
                QCOMPARE(error().kind(), ClientError::Kind::Network);
            }
        }
    }
    drainEvents();

    QCOMPARE(target.requestCount(), 0);
    QCOMPARE(origin.requestCount(), 1);
    delete reply.data();
    client.reset();
    drainEvents();
}

// The key must not surface anywhere a refused redirect can print or report --
// the error, the raw body handed to observers, or any log line, with every
// logging category switched on.
void TestRedirect::keyNeverLoggedDuringRefusedRedirect_data()
{
    QTest::addColumn<QString>("who");
    QTest::addColumn<bool>("otherHost");
    for (const char *who : {"bearer", "azure", "organization", "bearer-stream", "azure-stream"}) {
        QTest::addRow("%s/other-host", who) << QString::fromLatin1(who) << true;
        QTest::addRow("%s/other-port", who) << QString::fromLatin1(who) << false;
    }
}

void TestRedirect::keyNeverLoggedDuringRefusedRedirect()
{
    QFETCH(QString, who);
    QFETCH(bool, otherHost);

    const QString secret = QStringLiteral("sk-hardening-%1").arg(who);
    const Servers servers = makeServers(otherHost);
    std::optional<ClientError> error;
    QByteArray observedBody;
    {
        CapturedMessages captured;
        if (who == "organization") {
            Organization organization(servers.origin->baseUrl(), secret);
            const auto reply = awaited(organization.listProjects());
            QVERIFY(reply);
            error = reply->error();
        } else {
            Client client(servers.origin->baseUrl(), secret);
            client.setOrganization(QStringLiteral("org-hardening"));
            client.setAuthScheme(who.startsWith("azure") ? Client::AuthScheme::AzureApiKey
                                                         : Client::AuthScheme::BearerToken);
            if (who.endsWith("stream")) {
                error = issue(client, QStringLiteral("stream"));
            } else {
                ChatCompletionReply *reply = client.createChatCompletion(chatRequest());
                QObject::connect(reply, &ChatCompletionReply::responseReceived,
                                 [&](const QByteArray &body, int) { observedBody = body; });
                const auto owned = awaited(reply);
                QVERIFY(owned);
                error = owned->error();
            }
        }
        // The error as an application would print it, through qDebug.
        qDebug() << "error:" << error->message() << error->kind();
        const QString log = captured.all();
        QVERIFY2(log.contains(QStringLiteral("error:")), "the message handler captured nothing");
        QVERIFY2(!log.contains(secret), qPrintable(log));
    }
    QVERIFY(error);
    QCOMPARE(error->kind(), ClientError::Kind::Redirect);
    QVERIFY(!error->message().contains(secret));
    QVERIFY(!error->type().contains(secret) && !error->code().contains(secret));
    QVERIFY(!observedBody.contains(secret.toUtf8()));
    QCOMPARE(servers.targetRequests(), 0);
}

// A Location the HTTP stack cannot speak is not followed either -- Qt refuses it
// before the redirect policy is asked -- and fails as a network error (the
// narrower claim Client.h makes). Nothing reaches the Location.
void TestRedirect::nonHttpLocationFailsAsNetworkError()
{
    StubServer origin({redirectTo("file:///etc/hostname"), {chatBody()}});
    Client client(origin.baseUrl(), key);
    client.setRetryPolicy(RetryPolicy::none());

    const std::optional<ClientError> error = issue(client, QStringLiteral("post"));
    QVERIFY2(error, "the request never settled");
    QCOMPARE(error->kind(), ClientError::Kind::Network);
    QCOMPARE(origin.requestCount(), 1);
}

// A retry waiting out its back-off when the manager goes must not call the
// factory -- it captured that manager -- and must settle now, not when the
// back-off would have expired.
void TestRedirect::managerGoneDuringRetryBackoff_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("injected");
    for (const char *path : {"post", "multipart"}) {
        QTest::addRow("%s/delete-client", path) << QString::fromLatin1(path) << false;
        QTest::addRow("%s/delete-injected-manager", path) << QString::fromLatin1(path) << true;
    }
}

void TestRedirect::managerGoneDuringRetryBackoff()
{
    QFETCH(QString, path);
    QFETCH(bool, injected);

    StubServer origin({{QByteArray("{}"), 503, "application/json"}, {chatBody()}});
    auto client = std::make_unique<Client>(origin.baseUrl(), key);
    auto manager = std::make_unique<QNetworkAccessManager>();
    if (injected)
        client->setNetworkAccessManager(manager.get());
    RetryPolicy policy;
    policy.maxRetries = 1;
    policy.initialDelayMs = 60000;
    policy.maxDelayMs = 60000;
    policy.jitter = false;
    client->setRetryPolicy(policy);

    RestReplyBase *reply = nullptr;
    if (path == QLatin1String("post")) {
        reply = client->createChatCompletion(chatRequest());
    } else {
        reply = client->createTranscription(TranscriptionRequest(
                QByteArray("RIFFfake"), QStringLiteral("clip.wav"), QStringLiteral("whisper-1")));
    }
    reply->setAutoDelete(false);
    const std::unique_ptr<RestReplyBase> owner(reply);
    QSignalSpy retrying(reply, &RestReplyBase::retrying);
    QSignalSpy done(reply, &RestReplyBase::done);
    QVERIFY(retrying.wait(5000));

    if (injected)
        manager.reset();
    else
        client.reset();
    QCOMPARE(done.count(), 0);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QCOMPARE(done.count(), 1);
    QCOMPARE(reply->error().kind(), ClientError::Kind::Network);
    QCOMPARE(origin.requestCount(), 1);
}

// A call made and abandoned in the same turn -- the first REST attempt is
// deferred -- sends nothing once the Client is gone, and still settles.
void TestRedirect::clientGoneBeforeFirstAttempt_data()
{
    QTest::addColumn<QString>("path");
    QTest::newRow("post") << QStringLiteral("post");
    QTest::newRow("stream") << QStringLiteral("stream");
}

void TestRedirect::clientGoneBeforeFirstAttempt()
{
    QFETCH(QString, path);

    StubServer origin(chatBody());
    auto client = std::make_unique<Client>(origin.baseUrl(), key);
    std::function<bool()> finished;
    std::function<ClientError()> error;
    std::unique_ptr<QObject> owner;
    if (path == QLatin1String("stream")) {
        auto *reply = client->createChatCompletionStream(chatRequest());
        reply->setAutoDelete(false);
        owner.reset(reply);
        finished = [reply] { return reply->isFinished(); };
        error = [reply] { return reply->error(); };
    } else {
        auto *reply = client->createChatCompletion(chatRequest());
        reply->setAutoDelete(false);
        owner.reset(reply);
        finished = [reply] { return reply->isFinished(); };
        error = [reply] { return reply->error(); };
    }
    client.reset();

    QVERIFY(QTest::qWaitFor(finished, 5000));
    QCOMPARE(error().kind(), ClientError::Kind::Network);
    drainEvents();
    if (path == QLatin1String("post"))
        QCOMPARE(origin.requestCount(), 0);
}

// The manager owns the replies now, so a reply that is done with must not be
// left behind under it -- after success, after a retry, and for a stream.
void TestRedirect::noReplyAccumulatesUnderTheManager()
{
    QNetworkAccessManager manager;
    const auto replies = [&manager] {
        return manager.findChildren<QNetworkReply *>(QString(), Qt::FindDirectChildrenOnly);
    };
    {
        StubServer origin({{QByteArray("{}"), 503, "application/json"}, {chatBody()}});
        Client client(origin.baseUrl(), key);
        client.setNetworkAccessManager(&manager);
        RetryPolicy policy;
        policy.initialDelayMs = 0;
        policy.jitter = false;
        client.setRetryPolicy(policy);

        {
            const auto reply = awaited(client.createChatCompletion(chatRequest()));
            QVERIFY(reply);
            QVERIFY(reply->isSuccess());
            QCOMPARE(reply->retryCount(), 1);
        }
        {
            const auto reply = awaited(client.createChatCompletion(chatRequest()));
            QVERIFY(reply);
        }
        {
            origin.setResponses({{QByteArray("data: [DONE]\n\n"), 200, "text/event-stream"}});
            const auto reply = awaited(client.createChatCompletionStream(chatRequest()));
            QVERIFY(reply);
        }
        drainEvents();
    }
    QVERIFY2(replies().isEmpty(), qPrintable(QString::number(replies().size())));
}

QTEST_MAIN(TestRedirect)
#include "tst_redirect.moc"
