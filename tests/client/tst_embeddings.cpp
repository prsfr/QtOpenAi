// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

// A one-shot stub server returning a fixed body and recording the request line.
class StubServer : public QObject
{
    Q_OBJECT
public:
    explicit StubServer(QByteArray body, QObject *parent = nullptr)
        : QObject(parent)
        , m_body(std::move(body))
    {
        m_server.listen(QHostAddress::LocalHost, 0);
        connect(&m_server, &QTcpServer::newConnection, this, &StubServer::onConnection);
    }

    QUrl baseUrl() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1/v1").arg(m_server.serverPort()));
    }

    QByteArray requestLine() const { return m_requestLine; }
    QByteArray requestBody() const { return m_requestBody; }

private slots:
    void onConnection()
    {
        QTcpSocket *socket = m_server.nextPendingConnection();
        auto buffer = std::make_shared<QByteArray>();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer]() {
            *buffer += socket->readAll();
            if (!buffer->contains("\r\n\r\n"))
                return;
            if (m_requestLine.isEmpty()) {
                m_requestLine = buffer->left(buffer->indexOf("\r\n"));
                m_requestBody = buffer->mid(buffer->indexOf("\r\n\r\n") + 4);
            }

            QByteArray response = "HTTP/1.1 200 X\r\n"
                                  "Content-Type: application/json\r\n"
                                  "Content-Length: "
                                  + QByteArray::number(m_body.size())
                                  + "\r\n"
                                    "Connection: close\r\n\r\n"
                                  + m_body;
            socket->write(response);
            socket->flush();
            socket->disconnectFromHost();
        });
    }

private:
    QTcpServer m_server;
    QByteArray m_body;
    QByteArray m_requestLine;
    QByteArray m_requestBody;
};

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

    EmbeddingReply *reply = client.createEmbeddings(
            EmbeddingRequest(QStringLiteral("text-embedding-3-small"), QStringLiteral("hi")));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/embeddings "));
    QVERIFY(server.requestBody().contains("\"input\":\"hi\""));
    QCOMPARE(reply->response().firstVector().size(), 3);
    delete reply;
}

void TestEmbeddingsAndModelsClient::listModelsUsesGet()
{
    StubServer server(R"({"object":"list","data":[
        {"id":"gpt-4o","object":"model","created":1,"owned_by":"openai"},
        {"id":"gpt-4o-mini","object":"model","created":2,"owned_by":"openai"}]})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    ModelListReply *reply = client.listModels();
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/models "));
    QCOMPARE(reply->models().size(), 2);
    delete reply;
}

void TestEmbeddingsAndModelsClient::getModelUsesGet()
{
    StubServer server(R"({"id":"gpt-4o","object":"model","created":1,"owned_by":"openai"})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    ModelReply *reply = client.getModel(QStringLiteral("gpt-4o"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/models/gpt-4o "));
    QCOMPARE(reply->model().id(), QStringLiteral("gpt-4o"));
    QCOMPARE(reply->model().ownedBy(), QStringLiteral("openai"));
    delete reply;
}

QTEST_MAIN(TestEmbeddingsAndModelsClient)
#include "tst_embeddings.moc"
