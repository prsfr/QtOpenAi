// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

// Offline stub-server coverage for the Realtime REST endpoints (#25): the three
// ways to mint an ephemeral credential, and the SIP call control that answers a
// `realtime.call.incoming` webhook. The WebSocket channel itself is covered by
// tst_realtime_connection in the Realtime module.
class TestRealtimeClient : public QObject
{
    Q_OBJECT
private slots:
    void createClientSecretWrapsSessionAndExpiry();
    void createClientSecretOmitsUnsetExpiry();
    void createSessionPostsConfigDirectly();
    void createTranscriptionSessionUsesItsOwnPath();
    void createTranslationClientSecretUsesItsOwnPath();
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
