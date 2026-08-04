// SPDX-License-Identifier: MIT
#include <QtOpenAi/Tools/HttpTools.h>

#include <QtTest/QtTest>

#include "support/StubServer.h"

using namespace QtOpenAi::Tools;

// Coverage for the HTTP tool (#53). Everything runs against a local stub: a
// test for an egress guard must not itself egress.
class TestHttpTools : public QObject
{
    Q_OBJECT
private slots:
    void nothingIsFetchedByDefault();
    void anAllowedHostIsFetched();
    void aSubdomainIsNotTheHost();
    void httpsIsRequiredUnlessWaived();
    void theBodyIsCapped();
    void aRedirectIsNotFollowed();
    void anErrorStatusIsReportedNotReturned();
};

void TestHttpTools::nothingIsFetchedByDefault()
{
    // Deny by default, and by the strictest reading: an empty allow-list is not
    // "no filter", it is "no hosts".
    StubServer server(QByteArray("secret"));
    HttpTools tools;
    QVERIFY(tools.allowedHosts().isEmpty());
    QVERIFY(tools.requiresHttps());
    QSignalSpy refused(&tools, &HttpTools::refused);

    const QString url = server.baseUrl().toString();
    const QString result = tools.http_get(url);
    QVERIFY2(!result.contains(QStringLiteral("secret")), qPrintable(result));
    QCOMPARE(refused.count(), 1);
    QCOMPARE(server.requestCount(), 0);

    // The link-local metadata address, which is what a model asked to
    // "summarise this page" will happily be talked into fetching from inside
    // whatever network the application is running in.
    QVERIFY(tools.http_get(QStringLiteral("http://169.254.169.254/latest/meta-data/"))
                    .contains(QStringLiteral("https")));
    tools.setRequiresHttps(false);
    QVERIFY(tools.http_get(QStringLiteral("http://169.254.169.254/latest/meta-data/"))
                    .contains(QStringLiteral("not one of the hosts")));
    QVERIFY(tools.http_get(QStringLiteral("http://localhost:8080/admin"))
                    .contains(QStringLiteral("not one of the hosts")));

    QVERIFY(tools.http_get(QStringLiteral("not a url")).contains(QStringLiteral("not a valid")));
}

void TestHttpTools::anAllowedHostIsFetched()
{
    StubServer server(QByteArray("the page body"));
    HttpTools tools;
    tools.setRequiresHttps(false); // the stub speaks http; see the https test
    tools.addAllowedHost(QStringLiteral("127.0.0.1"));
    QSignalSpy fetched(&tools, &HttpTools::fetched);

    QCOMPARE(tools.http_get(server.baseUrl().toString()), QStringLiteral("the page body"));
    QCOMPARE(server.requestCount(), 1);
    QCOMPARE(fetched.count(), 1);

    // Normalised once, so a difference in case cannot become a difference in
    // policy.
    tools.setAllowedHosts({QStringLiteral("  127.0.0.1  ")});
    QCOMPARE(tools.allowedHosts(), QStringList({QStringLiteral("127.0.0.1")}));
}

void TestHttpTools::aSubdomainIsNotTheHost()
{
    // No wildcards: "example.com" allowing "evil.example.com" is the kind of
    // thing that looks harmless until someone registers the subdomain.
    HttpTools tools;
    tools.addAllowedHost(QStringLiteral("example.com"));
    QVERIFY(tools.http_get(QStringLiteral("https://evil.example.com/"))
                    .contains(QStringLiteral("not one of the hosts")));
    QVERIFY(tools.http_get(QStringLiteral("https://example.com.evil.test/"))
                    .contains(QStringLiteral("not one of the hosts")));
}

void TestHttpTools::httpsIsRequiredUnlessWaived()
{
    // Plaintext can be rewritten in transit by anyone on the path, and what
    // comes back is then instructions the model will read.
    HttpTools tools;
    tools.addAllowedHost(QStringLiteral("127.0.0.1"));
    QVERIFY(tools.http_get(QStringLiteral("http://127.0.0.1/page"))
                    .contains(QStringLiteral("only https")));
    // Waivable, because a local or on-premises service may not have TLS -- but
    // it has to be said out loud.
    tools.setRequiresHttps(false);
    QVERIFY(!tools.http_get(QStringLiteral("http://127.0.0.1:1/page"))
                     .contains(QStringLiteral("only https")));
}

void TestHttpTools::theBodyIsCapped()
{
    StubServer server(QByteArray(4096, 'x'));
    HttpTools tools;
    tools.setRequiresHttps(false);
    tools.addAllowedHost(QStringLiteral("127.0.0.1"));
    tools.setMaxBytes(64);
    QSignalSpy refused(&tools, &HttpTools::refused);

    const QString result = tools.http_get(server.baseUrl().toString());
    QVERIFY2(result.contains(QStringLiteral("larger")), qPrintable(result));
    // Refused, not truncated-and-returned: the caller gets no partial page it
    // might mistake for the whole one.
    QVERIFY(!result.contains(QStringLiteral("xxxx")));
    QCOMPARE(refused.count(), 1);

    // 0 means no limit.
    tools.setMaxBytes(0);
    QCOMPARE(tools.http_get(server.baseUrl().toString()).size(), 4096);
}

void TestHttpTools::aRedirectIsNotFollowed()
{
    // A redirect names a host the allow-list never approved. Following one
    // would hand the decision to the server.
    StubServer server(QList<StubServer::Response> {{QByteArray("moved"), 302}});
    HttpTools tools;
    tools.setRequiresHttps(false);
    tools.addAllowedHost(QStringLiteral("127.0.0.1"));

    QVERIFY(tools.http_get(server.baseUrl().toString())
                    .contains(QStringLiteral("does not follow")));
}

void TestHttpTools::anErrorStatusIsReportedNotReturned()
{
    StubServer server(QList<StubServer::Response> {{QByteArray("<h1>Not found</h1>"), 404}});
    HttpTools tools;
    tools.setRequiresHttps(false);
    tools.addAllowedHost(QStringLiteral("127.0.0.1"));

    const QString result = tools.http_get(server.baseUrl().toString());
    QCOMPARE(result, QStringLiteral("the server answered 404"));
    // The error page's own content is not handed to the model: a 404 body is a
    // page someone else wrote, and it is not the answer to anything.
    QVERIFY(!result.contains(QStringLiteral("Not found")));
}

QTEST_MAIN(TestHttpTools)
#include "tst_http.moc"
