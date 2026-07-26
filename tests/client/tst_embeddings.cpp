// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

class TestEmbeddingsAndModelsClient : public QObject
{
    Q_OBJECT
private slots:
    void createEmbeddingsPostsAndParses();
    void listModelsUsesGet();
    void getModelUsesGet();
};

void TestEmbeddingsAndModelsClient::createEmbeddingsPostsAndParses()
{
    StubServer server(R"({"object":"list","data":[
        {"object":"embedding","index":0,"embedding":[0.1,0.2,0.3]}],
        "model":"text-embedding-3-small","usage":{"prompt_tokens":1,"total_tokens":1}})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.createEmbeddings(
            EmbeddingRequest(QStringLiteral("text-embedding-3-small"), QStringLiteral("hi"))));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/embeddings "));
    QVERIFY(server.requestBody().contains("\"input\":\"hi\""));
    QCOMPARE(reply->response().firstVector().size(), 3);
}

void TestEmbeddingsAndModelsClient::listModelsUsesGet()
{
    StubServer server(R"({"object":"list","data":[
        {"id":"gpt-4o","object":"model","created":1,"owned_by":"openai"},
        {"id":"gpt-4o-mini","object":"model","created":2,"owned_by":"openai"}]})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.listModels());
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/models "));
    QCOMPARE(reply->models().size(), 2);
}

void TestEmbeddingsAndModelsClient::getModelUsesGet()
{
    StubServer server(R"({"id":"gpt-4o","object":"model","created":1,"owned_by":"openai"})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.getModel(QStringLiteral("gpt-4o")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/models/gpt-4o "));
    QCOMPARE(reply->model().id(), QStringLiteral("gpt-4o"));
    QCOMPARE(reply->model().ownedBy(), QStringLiteral("openai"));
}

QTEST_MAIN(TestEmbeddingsAndModelsClient)
#include "tst_embeddings.moc"
