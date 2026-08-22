// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

// Offline stub-server coverage for the Realtime REST endpoints (#25): the three
// ways to mint an ephemeral credential, opening a call over WebRTC, and the
// four control verbs that also serve a call arriving by SIP. The WebSocket
// channel itself is covered by tst_realtime_connection in the Realtime module.
class TestRealtimeClient : public QObject
{
    Q_OBJECT
private slots:
    void createClientSecretWrapsSessionAndExpiry();
    void createClientSecretOmitsUnsetExpiry();
    void createSessionPostsConfigDirectly();
    void createTranscriptionSessionUsesItsOwnPath();
    void createTranslationClientSecretUsesItsOwnPath();
    void createCallPostsOfferAndSessionAsTypedParts();
    void createCallPostsBareSdpWhenTheSessionComesFromTheToken();
    void createCallSurvivesAMissingLocation();
    void acceptCallPostsSessionConfig();
    void rejectCallSendsStatusCode();
    void rejectCallOmitsUnsetStatusCode();
    void hangupCallPostsEmptyBody();
    void referCallSendsTargetUri();
};

namespace {

RealtimeSessionConfig sampleSession()
{
    RealtimeSessionConfig config;
    config.setType(QStringLiteral("realtime"));
    config.setModel(QStringLiteral("gpt-realtime"));
    config.setInstructions(QStringLiteral("Be brief."));
    return config;
}

} // namespace

void TestRealtimeClient::createClientSecretWrapsSessionAndExpiry()
{
    StubServer server(QByteArray(R"({"value":"ek_123","expires_at":1756310470,)"
                                 R"("session":{"id":"sess_1","model":"gpt-realtime"}})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.createRealtimeClientSecret(sampleSession(), 600));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/realtime/client_secrets "));
    // The session travels nested under `session`, with the expiry beside it.
    QVERIFY(server.requestBody().contains(R"("session":{)"));
    QVERIFY(server.requestBody().contains(R"("model":"gpt-realtime")"));
    QVERIFY(server.requestBody().contains(R"("anchor":"created_at")"));
    QVERIFY(server.requestBody().contains(R"("seconds":600)"));
    QCOMPARE(reply->clientSecret().value(), QStringLiteral("ek_123"));
    QCOMPARE(reply->clientSecret().session().model(), QStringLiteral("gpt-realtime"));
}

void TestRealtimeClient::createClientSecretOmitsUnsetExpiry()
{
    StubServer server(QByteArray(R"({"value":"ek_123"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.createRealtimeClientSecret(sampleSession()));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    // No expiry given means the server's default, not a pinned one.
    QVERIFY(!server.requestBody().contains("expires_after"));
}

void TestRealtimeClient::createSessionPostsConfigDirectly()
{
    StubServer server(QByteArray(R"({"id":"sess_1","object":"realtime.session",)"
                                 R"("model":"gpt-realtime","expires_at":1742188264})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.createRealtimeSession(sampleSession()));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/realtime/sessions "));
    // Unlike /client_secrets this endpoint takes the configuration as the body
    // itself, and answers with the session object rather than a secret.
    QVERIFY(server.requestBody().startsWith(R"({"instructions":)"));
    QVERIFY(!server.requestBody().contains(R"("session":)"));
    QCOMPARE(reply->session().id(), QStringLiteral("sess_1"));
    QCOMPARE(reply->session().expiresAt(), Q_INT64_C(1742188264));
}

void TestRealtimeClient::createTranscriptionSessionUsesItsOwnPath()
{
    // The pre-GA endpoint nests the key under `client_secret`; the value type
    // accepts that spelling, so the same reply serves both.
    StubServer server(QByteArray(R"({"client_secret":{"value":"ek_123",)"
                                 R"("expires_at":1742188264},"model":"gpt-4o-transcribe"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    RealtimeSessionConfig config;
    config.setType(QStringLiteral("transcription"));
    const auto reply = awaited(client.createRealtimeTranscriptionSession(config));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/realtime/transcription_sessions "));
    QVERIFY(server.requestBody().contains(R"("type":"transcription")"));
    QCOMPARE(reply->clientSecret().value(), QStringLiteral("ek_123"));
}

void TestRealtimeClient::createTranslationClientSecretUsesItsOwnPath()
{
    StubServer server(QByteArray(R"({"value":"ek_123"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.createRealtimeTranslationClientSecret(sampleSession()));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/realtime/translations/client_secrets "));
    QVERIFY(server.requestBody().contains(R"("session":{)"));
}

namespace {

// The answer the API sends back, trimmed to the shape that matters here.
constexpr auto kSdpAnswer = "v=0\r\no=- 4227147428 1719357865 IN IP4 127.0.0.1\r\ns=-\r\n";

StubServer::Response createdCall(const QByteArray &location)
{
    StubServer::Response response {kSdpAnswer, 201, "application/sdp", {}};
    if (!location.isEmpty())
        response.headers.append({"Location", location});
    return response;
}

} // namespace

void TestRealtimeClient::createCallPostsOfferAndSessionAsTypedParts()
{
    StubServer server(QList<StubServer::Response> {createdCall("/v1/realtime/calls/rtc_abc123")});
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply
            = awaited(client.createRealtimeCall(QByteArray("v=0\r\no=offer\r\n"), sampleSession()));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/realtime/calls "));
    QVERIFY(server.requestHeaders().toLower().contains("content-type: multipart/form-data;"));

    // Two parts, each with its own content type and *neither* a file: the
    // endpoint dispatches on those types, and a plain form field cannot carry
    // one. A filename here would be wrong rather than merely redundant.
    const QByteArray body = server.requestBody();
    QVERIFY2(body.contains("name=\"sdp\"\r\n"), body.constData());
    QVERIFY2(body.contains("name=\"session\"\r\n"), body.constData());
    QVERIFY(!body.contains("filename="));
    QVERIFY(body.toLower().contains("content-type: application/sdp"));
    QVERIFY(body.toLower().contains("content-type: application/json"));
    QVERIFY(body.contains(R"("model":"gpt-realtime")"));
    QVERIFY(body.contains("o=offer"));

    // 201 is a success, and the result is in two places at once.
    QCOMPARE(reply->sdpAnswer(), QByteArray(kSdpAnswer));
    QCOMPARE(reply->callId(), QStringLiteral("rtc_abc123"));
}

void TestRealtimeClient::createCallPostsBareSdpWhenTheSessionComesFromTheToken()
{
    StubServer server(QList<StubServer::Response> {createdCall("/v1/realtime/calls/rtc_xyz")});
    Client client(server.baseUrl(), QStringLiteral("ek_ephemeral"));

    // No session: the credential is a client secret, which already carries one.
    // This is the only variant that endpoint accepts from a client secret, and
    // it is not multipart at all.
    const auto reply = awaited(client.createRealtimeCall(QByteArray("v=0\r\no=offer\r\n")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY2(server.requestHeaders().toLower().contains("content-type: application/sdp"),
             server.requestHeaders().constData());
    QCOMPARE(server.requestBody(), QByteArray("v=0\r\no=offer\r\n"));
    QCOMPARE(reply->callId(), QStringLiteral("rtc_xyz"));
}

void TestRealtimeClient::createCallSurvivesAMissingLocation()
{
    // No Location header. The media path still works, so this is not a failure
    // -- the caller simply has a call it cannot address, and finding that out
    // through an empty callId() beats finding it out through a decode error.
    StubServer server(QList<StubServer::Response> {createdCall({})});
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.createRealtimeCall(QByteArray("v=0\r\n"), sampleSession()));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(reply->callId().isEmpty());
    QCOMPARE(reply->sdpAnswer(), QByteArray(kSdpAnswer));
}

void TestRealtimeClient::acceptCallPostsSessionConfig()
{
    StubServer server(QByteArray("{}"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply
            = awaited(client.acceptRealtimeCall(QStringLiteral("call_1"), sampleSession()));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/realtime/calls/call_1/accept "));
    QVERIFY(server.requestBody().contains(R"("model":"gpt-realtime")"));
}

void TestRealtimeClient::rejectCallSendsStatusCode()
{
    StubServer server(QByteArray("{}"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.rejectRealtimeCall(QStringLiteral("call_1"), 486));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/realtime/calls/call_1/reject "));
    QCOMPARE(server.requestBody(), QByteArray(R"({"status_code":486})"));
}

void TestRealtimeClient::rejectCallOmitsUnsetStatusCode()
{
    StubServer server(QByteArray("{}"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.rejectRealtimeCall(QStringLiteral("call_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    // Omitting the code lets the API answer its documented default (603).
    QCOMPARE(server.requestBody(), QByteArray("{}"));
}

void TestRealtimeClient::hangupCallPostsEmptyBody()
{
    StubServer server(QByteArray("{}"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.hangupRealtimeCall(QStringLiteral("call_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/realtime/calls/call_1/hangup "));
}

void TestRealtimeClient::referCallSendsTargetUri()
{
    StubServer server(QByteArray("{}"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(
            client.referRealtimeCall(QStringLiteral("call_1"), QStringLiteral("tel:+14155550123")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/realtime/calls/call_1/refer "));
    QCOMPARE(server.requestBody(), QByteArray(R"({"target_uri":"tel:+14155550123"})"));
}

QTEST_MAIN(TestRealtimeClient)
#include "tst_realtime.moc"
