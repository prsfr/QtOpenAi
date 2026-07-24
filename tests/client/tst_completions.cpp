// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/StubServer.h"

class TestCompletionsClient : public QObject
{
    Q_OBJECT
private slots:
    void createPostsAndParses();
    void networkErrorIsReported();
};

void TestCompletionsClient::createPostsAndParses()
{
    StubServer server(R"({"id":"cmpl_1","object":"text_completion","created":1,
        "model":"davinci-002","choices":[{"text":" world","index":0,"logprobs":null,
        "finish_reason":"stop"}],"usage":{"prompt_tokens":1,"completion_tokens":1,
        "total_tokens":2}})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    CompletionReply *reply = client.createCompletion(
            CompletionRequest(QStringLiteral("davinci-002"), QStringLiteral("Hello")));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/completions "));
    QVERIFY(server.requestBody().contains("\"prompt\":\"Hello\""));
    QCOMPARE(reply->response().firstText(), QStringLiteral(" world"));
    delete reply;
}

void TestCompletionsClient::networkErrorIsReported()
{
    // Point at a closed port to exercise the failure path.
    Client client(QUrl(QStringLiteral("http://127.0.0.1:1/v1")), QStringLiteral("k"));
    client.setRetryPolicy(RetryPolicy::none());

    CompletionReply *reply
            = client.createCompletion(CompletionRequest(QStringLiteral("m"), QStringLiteral("x")));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(!reply->isSuccess());
    QVERIFY(reply->error().isError());
    delete reply;
}

QTEST_MAIN(TestCompletionsClient)
#include "tst_completions.moc"
