// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtCore/QPointer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/StubServer.h"

// The rules every endpoint call obeys, asserted once.
//
// Building a request used to be spelled out in each of the ~100 endpoint
// methods, so these rules were only ever true by repetition. They now live in
// the four ClientPrivate request helpers, which makes them worth pinning down:
// a single edit there changes every endpoint at once.
class TestRequestPlumbing : public QObject
{
    Q_OBJECT
private slots:
    void doesNotOwnAnInjectedNetworkAccessManager();
    void ownsTheNetworkAccessManagerItCreates();
    void streamingRequestsAskForServerSentEvents();
    void streamingLeavesTheCallersRequestAlone();
    void queryParametersMergeWithTheAzureApiVersion();
};

// setNetworkAccessManager() is documented not to take ownership. Destroying the
// client must therefore leave the caller's manager alive.
void TestRequestPlumbing::doesNotOwnAnInjectedNetworkAccessManager()
{
    auto *manager = new QNetworkAccessManager;
    const QPointer<QNetworkAccessManager> alive(manager);
    {
        Client client;
        client.setNetworkAccessManager(manager);
        QCOMPARE(client.networkAccessManager(), manager);
    }
    QVERIFY2(!alive.isNull(), "the client deleted a manager it does not own");
    delete manager;
}

// The manager the client creates on first use is parented to it, so it is freed
// with the client and never leaks.
void TestRequestPlumbing::ownsTheNetworkAccessManagerItCreates()
{
    QPointer<QNetworkAccessManager> created;
    {
        Client client;
        created = client.networkAccessManager();
        QVERIFY(!created.isNull());
        QCOMPARE(created->parent(), &client);
        // The same instance is reused for every later call.
        QCOMPARE(client.networkAccessManager(), created.data());
    }
    QVERIFY2(created.isNull(), "the client leaked the manager it created");
}

void TestRequestPlumbing::streamingRequestsAskForServerSentEvents()
{
    StubServer server(QByteArray("data: [DONE]\n\n"), QByteArray("text/event-stream"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ChatCompletionRequest request;
    request.setModel(QStringLiteral("gpt-4o"));

    auto *reply = client.createChatCompletionStream(request);
    QSignalSpy doneSpy(reply, &ChatCompletionStreamReply::done);
    QVERIFY(doneSpy.wait(5000));

    QVERIFY2(server.requestHeaders().toLower().contains("accept: text/event-stream"),
             server.requestHeaders().constData());
    // Streaming is forced on regardless of what the caller set.
    QVERIFY(server.requestBody().contains("\"stream\":true"));
}

// The client streams from a copy, so a request object can be reused for a
// non-streaming call afterwards without silently having become a streaming one.
void TestRequestPlumbing::streamingLeavesTheCallersRequestAlone()
{
    StubServer server(QByteArray("data: [DONE]\n\n"), QByteArray("text/event-stream"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ChatCompletionRequest request;
    request.setModel(QStringLiteral("gpt-4o"));
    QVERIFY(!request.stream());

    auto *reply = client.createChatCompletionStream(request);
    QSignalSpy doneSpy(reply, &ChatCompletionStreamReply::done);
    QVERIFY(doneSpy.wait(5000));

    QVERIFY2(!request.stream(), "createChatCompletionStream() mutated the caller's request");
    QVERIFY(!request.toJson().contains(QStringLiteral("stream")));
}

// Azure endpoints carry an api-version parameter on every URL. Pagination adds
// to that query rather than replacing it.
void TestRequestPlumbing::queryParametersMergeWithTheAzureApiVersion()
{
    StubServer server(QByteArray(R"({"object":"list","data":[],"has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));
    client.setApiVersion(QStringLiteral("2024-06-01"));

    ListParams params;
    params.limit = 7;
    params.after = QStringLiteral("file-1");

    auto *reply = client.listFiles(params);
    QSignalSpy doneSpy(reply, &FileListReply::done);
    QVERIFY(doneSpy.wait(5000));

    const QByteArray line = server.requestLine();
    QVERIFY2(line.contains("api-version=2024-06-01"), line.constData());
    QVERIFY2(line.contains("limit=7"), line.constData());
    QVERIFY2(line.contains("after=file-1"), line.constData());
}

QTEST_MAIN(TestRequestPlumbing)
#include "tst_requestplumbing.moc"
