// SPDX-License-Identifier: MIT
#include <QtOpenAi/Admin/Organization.h>
#include <QtOpenAi/Client/Client.h>

#include <QtTest/QtTest>

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

QTEST_MAIN(TestRedirect)
#include "tst_redirect.moc"
