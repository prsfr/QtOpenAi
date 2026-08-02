// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/ProviderProfile.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

namespace {

const char kCompletion[] = R"({"id":"c","object":"chat.completion","created":1,
    "model":"m","choices":[{"index":0,"finish_reason":"stop",
    "message":{"role":"assistant","content":"hi"}}]})";

ChatCompletionRequest sampleRequest()
{
    return ChatCompletionRequest(QStringLiteral("m"), {Message::user(QStringLiteral("hi"))});
}

// Apply a profile, then point it at the stub so the request is observable --
// the URL is checked separately, since redirecting it is the only way to see
// what the rest of the profile did.
QByteArray requestThrough(ProviderProfile profile, StubServer &server, QByteArray *line = nullptr)
{
    Client client;
    client.setProfile(profile);
    client.setBaseUrl(server.baseUrl());
    client.setApiKey(QStringLiteral("k"));

    const auto reply = awaited(client.createChatCompletion(sampleRequest()));
    if (!reply)
        return {};
    if (line)
        *line = server.requestLine();
    return server.requestHeaders();
}

} // namespace

// Coverage for provider profiles (#47). A profile is only a bundle of settings,
// so what matters is that applying one puts the right bytes on the wire.
class TestProviderProfile : public QObject
{
    Q_OBJECT
private slots:
    void openAiSendsABearerToken();
    void azureSendsTheKeyHeaderAndApiVersion();
    void localProvidersNeedNoKey();
    void profileHeadersReachTheRequest();
    void aCustomProfileIsJustAProfile();
    void everyBuiltInHasAUsableUrl();
    void looksUpABuiltInByName();
    void applyingAProfileLeavesTheKeyAlone();
};

void TestProviderProfile::openAiSendsABearerToken()
{
    const ProviderProfile profile = ProviderProfile::openAi();
    QCOMPARE(profile.baseUrl(), QUrl(QStringLiteral("https://api.openai.com/v1")));
    QCOMPARE(profile.authScheme(), Client::AuthScheme::BearerToken);
    QVERIFY(profile.requiresApiKey());
    QVERIFY(!profile.defaultModel().isEmpty());

    StubServer server(kCompletion);
    const QByteArray headers = requestThrough(profile, server);
    QVERIFY(headers.contains("Authorization: Bearer k"));
    QVERIFY(!headers.contains("api-key:"));
}

void TestProviderProfile::azureSendsTheKeyHeaderAndApiVersion()
{
    const ProviderProfile profile = ProviderProfile::azure(QStringLiteral("contoso"));
    QCOMPARE(profile.baseUrl(), QUrl(QStringLiteral("https://contoso.openai.azure.com/openai/v1")));
    QVERIFY(!profile.apiVersion().isEmpty());

    StubServer server(kCompletion);
    QByteArray line;
    const QByteArray headers = requestThrough(profile, server, &line);

    // Azure presents the key in its own header, not as a bearer token ...
    QVERIFY(headers.contains("api-key: k"));
    QVERIFY(!headers.contains("Authorization: Bearer"));
    // ... and needs the api-version parameter on every request.
    QVERIFY(line.contains("api-version=" + profile.apiVersion().toUtf8()));

    // The version is a date that keeps moving, so it is an argument.
    QCOMPARE(ProviderProfile::azure(QStringLiteral("contoso"), QStringLiteral("2030-01-01"))
                     .apiVersion(),
             QStringLiteral("2030-01-01"));
}

void TestProviderProfile::localProvidersNeedNoKey()
{
    // Prompting for a key the user does not have is worse than not prompting.
    for (const ProviderProfile &profile :
         {ProviderProfile::ollama(), ProviderProfile::lmStudio(), ProviderProfile::vllm()}) {
        QVERIFY2(!profile.requiresApiKey(), qPrintable(profile.name()));
        QCOMPARE(profile.baseUrl().host(), QStringLiteral("localhost"));
    }

    // The hosted ones do.
    QVERIFY(ProviderProfile::groq().requiresApiKey());
    QVERIFY(ProviderProfile::openRouter().requiresApiKey());
}

void TestProviderProfile::profileHeadersReachTheRequest()
{
    ProviderProfile profile = ProviderProfile::openRouter();
    profile.setHeader("HTTP-Referer", "https://example.test");
    profile.setHeader("X-Title", "QtOpenAi");

    StubServer server(kCompletion);
    const QByteArray headers = requestThrough(profile, server);

    QVERIFY(headers.contains("HTTP-Referer: https://example.test"));
    QVERIFY(headers.contains("X-Title: QtOpenAi"));
}

void TestProviderProfile::aCustomProfileIsJustAProfile()
{
    // Nothing about the built-ins is privileged: a provider this library has
    // never heard of is the same type with its fields set.
    ProviderProfile profile;
    QVERIFY(profile.isNull());

    profile.setName(QStringLiteral("House LLM"));
    profile.setBaseUrl(QUrl(QStringLiteral("https://llm.internal/v1")));
    profile.setAuthScheme(Client::AuthScheme::AzureApiKey);
    profile.setHeader("X-Tenant", "acme");
    profile.setRequiresApiKey(false);
    QVERIFY(!profile.isNull());

    StubServer server(kCompletion);
    const QByteArray headers = requestThrough(profile, server);
    QVERIFY(headers.contains("api-key: k"));
    QVERIFY(headers.contains("X-Tenant: acme"));

    // A value type: copies compare equal and stay independent.
    ProviderProfile copy = profile;
    QCOMPARE(copy, profile);
    copy.setName(QStringLiteral("Other"));
    QVERIFY(copy != profile);
}

void TestProviderProfile::everyBuiltInHasAUsableUrl()
{
    const QList<ProviderProfile> profiles = ProviderProfile::builtIn();
    QVERIFY(!profiles.isEmpty());

    for (const ProviderProfile &profile : profiles) {
        QVERIFY2(!profile.name().isEmpty(), "a profile without a name cannot be offered");
        QVERIFY2(profile.baseUrl().isValid(), qPrintable(profile.name()));
        QVERIFY2(!profile.baseUrl().host().isEmpty(), qPrintable(profile.name()));
        QVERIFY2(!profile.isNull(), qPrintable(profile.name()));
    }
}

void TestProviderProfile::looksUpABuiltInByName()
{
    QCOMPARE(ProviderProfile::fromName(QStringLiteral("Ollama")), ProviderProfile::ollama());
    // Case-insensitive, because it comes out of a config file or a combo box.
    QCOMPARE(ProviderProfile::fromName(QStringLiteral("groq")), ProviderProfile::groq());
    // An unknown name is a null profile rather than a wrong one.
    QVERIFY(ProviderProfile::fromName(QStringLiteral("nope")).isNull());
}

void TestProviderProfile::applyingAProfileLeavesTheKeyAlone()
{
    // A profile says which provider; a key says who you are. Keeping the secret
    // out of a value type that gets copied and logged is the point.
    Client client;
    client.setApiKey(QStringLiteral("secret"));
    client.setProfile(ProviderProfile::groq());

    QCOMPARE(client.apiKey(), QStringLiteral("secret"));
    QCOMPARE(client.baseUrl(), ProviderProfile::groq().baseUrl());
    QCOMPARE(client.authScheme(), Client::AuthScheme::BearerToken);

    // Switching provider replaces what a profile owns ...
    client.setProfile(ProviderProfile::azure(QStringLiteral("contoso")));
    QCOMPARE(client.authScheme(), Client::AuthScheme::AzureApiKey);
    QVERIFY(!client.apiVersion().isEmpty());
    // ... and switching back clears what only the previous one needed.
    client.setProfile(ProviderProfile::openAi());
    QVERIFY(client.apiVersion().isEmpty());
}

QTEST_MAIN(TestProviderProfile)
#include "tst_providerprofile.moc"
