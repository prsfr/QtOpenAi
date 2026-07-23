// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

// A one-shot stub server that reads a full Content-Length-delimited request
// (so a multipart body is captured) and returns a fixed JSON body.
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
    QByteArray requestHeaders() const { return m_headers; }
    QByteArray requestBody() const { return m_requestBody; }

private slots:
    void onConnection()
    {
        QTcpSocket *socket = m_server.nextPendingConnection();
        auto buffer = std::make_shared<QByteArray>();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer]() {
            *buffer += socket->readAll();
            const int headerEnd = buffer->indexOf("\r\n\r\n");
            if (headerEnd < 0)
                return;
            const QByteArray head = buffer->left(headerEnd);
            int contentLength = 0;
            for (const QByteArray &line : head.split('\n')) {
                const QByteArray l = line.trimmed().toLower();
                if (l.startsWith("content-length:"))
                    contentLength = l.mid(15).trimmed().toInt();
            }
            if (buffer->size() < headerEnd + 4 + contentLength)
                return;

            if (m_requestLine.isEmpty()) {
                m_requestLine = buffer->left(buffer->indexOf("\r\n"));
                m_headers = head;
                m_requestBody = buffer->mid(headerEnd + 4);
            }

            QByteArray response = "HTTP/1.1 200 OK\r\n"
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
    QByteArray m_headers;
    QByteArray m_requestBody;
};

class TestImagesClient : public QObject
{
    Q_OBJECT
private slots:
    void generationPostsJsonAndParsesB64();
    void editUploadsMultipartWithMask();
};

void TestImagesClient::generationPostsJsonAndParsesB64()
{
    StubServer server(R"({"created":1,"data":[{"b64_json":"aGVsbG8="}]})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    ImageGenerationRequest request(QStringLiteral("a red cube"), QStringLiteral("gpt-image-1"));
    request.setSize(QStringLiteral("1024x1024"));

    ImageReply *reply = client.createImage(request);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/images/generations "));
    QVERIFY(server.requestBody().contains("\"prompt\":\"a red cube\""));
    QCOMPARE(reply->response().firstImage().b64Json(), QStringLiteral("aGVsbG8="));
    delete reply;
}

void TestImagesClient::editUploadsMultipartWithMask()
{
    StubServer server(R"({"created":2,"data":[{"url":"https://x/y.png"}]})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    ImageEditRequest request(QByteArray("PNGsourcebytes"), QStringLiteral("in.png"),
                             QStringLiteral("add a hat"), QStringLiteral("gpt-image-1"));
    request.setMask(QStringLiteral("mask.png"), QByteArray("PNGmaskbytes"));

    ImageReply *reply = client.createImageEdit(request);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/images/edits "));
    QVERIFY(server.requestHeaders().toLower().contains("content-type: multipart/form-data;"));
    const QByteArray body = server.requestBody();
    QVERIFY(body.contains("name=\"image\"; filename=\"in.png\""));
    QVERIFY(body.contains("PNGsourcebytes"));
    QVERIFY(body.contains("name=\"mask\"; filename=\"mask.png\""));
    QVERIFY(body.contains("PNGmaskbytes"));
    QVERIFY(body.contains("name=\"prompt\""));
    QCOMPARE(reply->response().firstImage().url(), QStringLiteral("https://x/y.png"));
    delete reply;
}

QTEST_MAIN(TestImagesClient)
#include "tst_images.moc"
