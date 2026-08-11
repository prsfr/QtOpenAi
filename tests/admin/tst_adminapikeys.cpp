// SPDX-License-Identifier: MIT
#include <QtOpenAi/Admin/Organization.h>
#include <QtOpenAi/Client/LoggingInterceptor.h>
#include <QtOpenAi/Core/AdminApiKey.h>

#include <QtTest/QtTest>

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Admin;
using QtOpenAi::Client::LoggingInterceptor;

namespace {

// The secret the API returns exactly once. Spelled here so every assertion in
// this file can say "and this string is not in that".
constexpr QLatin1String kSecret("sk-admin-1234abcdSECRET");

QByteArray createdKey()
{
    return R"({"object":"organization.admin_api_key","id":"key_new","name":"Deploy",
        "redacted_value":"sk-admin...def","created_at":1711471533,"expires_at":1714063533,
        "value":"sk-admin-1234abcdSECRET",
        "owner":{"type":"user","object":"organization.user","id":"user_1","name":"Ada",
                 "created_at":1711471533,"role":"owner"}})";
}

// The same key as a listing sees it: no `value`, and an explicit null for the
// two timestamps that need not exist.
QByteArray keyPage()
{
    return R"({"object":"list","data":[
        {"object":"organization.admin_api_key","id":"key_new","name":"Deploy",
         "redacted_value":"sk-admin...def","created_at":1711471533,"expires_at":null,
         "last_used_at":null,
         "owner":{"type":"user","object":"organization.user","id":"user_1","name":"Ada",
                  "created_at":1711471533,"role":"owner"}},
        {"object":"organization.admin_api_key","id":"key_old","name":null,
         "redacted_value":"sk-admin...xyz","created_at":1611471533,"expires_at":1614063533,
         "last_used_at":1611471999,
         "owner":{"type":"user","object":"organization.user","id":"user_2","name":"Grace",
                  "created_at":1611471533,"role":"owner"}}],
        "first_id":"key_new","last_id":"key_old","has_more":false})";
}

} // namespace

// Coverage for the organization admin API keys (#104, under #28). Offline:
// every request goes to the local stub server.
class TestAdminApiKeys : public QObject
{
    Q_OBJECT
private slots:
    void roundTripThroughJson();
    void aListedKeyCarriesNoSecret();
    void createReturnsTheSecretExactlyOnce();
    void createSendsAnExpiryOnlyWhenAsked_data();
    void createSendsAnExpiryOnlyWhenAsked();
    void listPassesThePaginationQuery();
    void getAndDeleteOneKey();
    void theLoggerNeverWritesTheCreatedSecret();
    void bodyRedactionKeepsEverythingElseReadable();
};

void TestAdminApiKeys::roundTripThroughJson()
{
    const AdminApiKey key = AdminApiKey::fromJson(QJsonDocument::fromJson(createdKey()).object());

    QCOMPARE(key.id(), QStringLiteral("key_new"));
    QCOMPARE(key.name(), QStringLiteral("Deploy"));
    QCOMPARE(key.redactedValue(), QStringLiteral("sk-admin...def"));
    QCOMPARE(key.createdAt(), qint64(1711471533));
    QCOMPARE(key.expiresAt(), qint64(1714063533));
    QVERIFY(key.owner().isUser());
    QCOMPARE(key.owner().user().name(), QStringLiteral("Ada"));

    // The secret survives the round trip, because a caller has to be able to
    // hand the created key to their own storage.
    QCOMPARE(key.value(), QString(kSecret));
    QVERIFY(key.hasValue());
    QCOMPARE(AdminApiKey::fromJson(key.toJson()), key);
}

void TestAdminApiKeys::aListedKeyCarriesNoSecret()
{
    const AdminApiKeyList page
            = AdminApiKeyList::fromJson(QJsonDocument::fromJson(keyPage()).object());
    QCOMPARE(page.size(), 2);
    QVERIFY(!page.hasMore);

    // An empty value() here is the API saying "not here", not a decode that
    // went wrong -- the whole reason hasValue() exists.
    for (const AdminApiKey &key : page.data) {
        QVERIFY2(!key.hasValue(), qPrintable(key.id()));
        QVERIFY(key.value().isEmpty());
        QVERIFY(!key.redactedValue().isEmpty());
    }

    // A key that never expires and has never been used sends null for both, and
    // 0 is how that reads -- not 1970, which would sort it as the oldest key in
    // the organization.
    const AdminApiKey fresh = page.data.at(0);
    QCOMPARE(fresh.expiresAt(), qint64(0));
    QCOMPARE(fresh.lastUsedAt(), qint64(0));
    QVERIFY(!fresh.toJson().contains(QStringLiteral("expires_at")));
    QVERIFY(!fresh.toJson().contains(QStringLiteral("last_used_at")));

    // A null name decodes as empty rather than the string "null".
    QVERIFY(page.data.at(1).name().isEmpty());
    QCOMPARE(page.data.at(1).lastUsedAt(), qint64(1611471999));
}

void TestAdminApiKeys::createReturnsTheSecretExactlyOnce()
{
    StubServer server(createdKey());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(organization.createAdminApiKey(QStringLiteral("Deploy")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(server.requestLine(), "POST /v1/organization/admin_api_keys HTTP/1.1");
    QVERIFY(server.requestHeaders().contains("Authorization: Bearer sk-admin-test"));

    QCOMPARE(reply->apiKey().value(), QString(kSecret));
    QVERIFY(reply->apiKey().hasValue());
}

void TestAdminApiKeys::createSendsAnExpiryOnlyWhenAsked_data()
{
    QTest::addColumn<int>("expiresInSeconds");
    QTest::addColumn<QByteArray>("body");

    // Zero means "does not expire", and the field is left out rather than sent
    // as a zero the server would read as "expired one second ago".
    QTest::newRow("no expiry") << 0 << QByteArray(R"({"name":"Deploy"})");
    QTest::newRow("negative is no expiry") << -5 << QByteArray(R"({"name":"Deploy"})");
    QTest::newRow("thirty days") << 2592000
                                 << QByteArray(R"({"expires_in_seconds":2592000,"name":"Deploy"})");
}

void TestAdminApiKeys::createSendsAnExpiryOnlyWhenAsked()
{
    QFETCH(int, expiresInSeconds);
    QFETCH(QByteArray, body);

    StubServer server(createdKey());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    QVERIFY(awaited(organization.createAdminApiKey(QStringLiteral("Deploy"), expiresInSeconds)));
    QCOMPARE(server.requestBody(), body);
}

void TestAdminApiKeys::listPassesThePaginationQuery()
{
    StubServer server(keyPage());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    QtOpenAi::Client::ListParams params;
    params.after = QStringLiteral("key_new");
    params.limit = 5;
    params.order = QStringLiteral("desc");

    const auto reply = awaited(organization.listAdminApiKeys(params));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(server.requestLine(),
             "GET /v1/organization/admin_api_keys?after=key_new&limit=5&order=desc HTTP/1.1");
    QCOMPARE(reply->apiKeys().size(), 2);
    QCOMPARE(reply->apiKeys().firstId, QStringLiteral("key_new"));
}

void TestAdminApiKeys::getAndDeleteOneKey()
{
    StubServer read(createdKey());
    Organization organization(read.baseUrl(), QStringLiteral("sk-admin-test"));

    QVERIFY(awaited(organization.getAdminApiKey(QStringLiteral("key_new"))));
    QCOMPARE(read.requestLine(), "GET /v1/organization/admin_api_keys/key_new HTTP/1.1");

    StubServer removed(R"({"id":"key_new","object":"organization.admin_api_key.deleted",
        "deleted":true})");
    Organization other(removed.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(other.deleteAdminApiKey(QStringLiteral("key_new")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(removed.requestLine(), "DELETE /v1/organization/admin_api_keys/key_new HTTP/1.1");
    QVERIFY(reply->apiKey().isDeleted());
    QCOMPARE(reply->apiKey().id(), QStringLiteral("key_new"));
}

void TestAdminApiKeys::theLoggerNeverWritesTheCreatedSecret()
{
    // The acceptance criterion of #104, and the one that needed a fix rather
    // than a check: header redaction never looked at bodies, so the one
    // response that carries a live credential was being written out verbatim.
    LoggingInterceptor logger;
    QStringList lines;
    connect(&logger, &LoggingInterceptor::logged, &logger,
            [&lines](const QString &line) { lines.append(line); });

    StubServer server(createdKey());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));
    organization.addInterceptor(&logger);

    // Bodies on and no truncation, so nothing but the redaction can be what
    // keeps the secret out of the log.
    logger.setLogBodies(true);
    logger.setMaxBodyLength(0);

    const auto reply = awaited(organization.createAdminApiKey(QStringLiteral("Deploy")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    // The caller still gets it...
    QCOMPARE(reply->apiKey().value(), QString(kSecret));

    // ...and the log does not, anywhere in it.
    const QString written = lines.join(QLatin1Char('\n'));
    QVERIFY2(!written.contains(QString(kSecret)), qPrintable(written));
    QVERIFY2(written.contains(QStringLiteral("<redacted>")), qPrintable(written));
    // The admin key in the request header is redacted by the older mechanism,
    // and still is.
    QVERIFY2(!written.contains(QStringLiteral("Bearer sk-admin-test")), qPrintable(written));
}

void TestAdminApiKeys::bodyRedactionKeepsEverythingElseReadable()
{
    // Redacting by key name rather than by searching the text: the point is that
    // a body logged for debugging is still worth reading afterwards.
    LoggingInterceptor logger;
    QStringList lines;
    connect(&logger, &LoggingInterceptor::logged, &logger,
            [&lines](const QString &line) { lines.append(line); });

    StubServer server(createdKey());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));
    organization.addInterceptor(&logger);
    logger.setLogBodies(true);
    logger.setMaxBodyLength(0);

    QVERIFY(awaited(organization.createAdminApiKey(QStringLiteral("Deploy"))));

    const QString written = lines.join(QLatin1Char('\n'));
    // Everything that is not the secret survives -- including redacted_value,
    // which exists precisely to be shown.
    QVERIFY2(written.contains(QStringLiteral("key_new")), qPrintable(written));
    QVERIFY2(written.contains(QStringLiteral("sk-admin...def")), qPrintable(written));
    QVERIFY2(written.contains(QStringLiteral("Ada")), qPrintable(written));
    // And the request body, which names the key, is readable too.
    QVERIFY2(written.contains(QStringLiteral("Deploy")), qPrintable(written));
}

QTEST_MAIN(TestAdminApiKeys)
#include "tst_adminapikeys.moc"
