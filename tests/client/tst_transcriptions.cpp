// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

// A one-shot stub server that reads a full Content-Length-delimited request
// (so the multipart body is captured) and returns a fixed JSON body.
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
            // Wait until the whole declared body has arrived.
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

class TestTranscriptionsClient : public QObject
{
    Q_OBJECT
private slots:
    void uploadsMultipartAndParsesVerboseJson();
};

void TestTranscriptionsClient::uploadsMultipartAndParsesVerboseJson()
{
    StubServer server(R"({"task":"transcribe","language":"english","duration":1.5,
        "text":"Hello world",
        "segments":[{"id":0,"start":0.0,"end":1.5,"text":"Hello world","avg_logprob":-0.2}]})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    TranscriptionRequest request(QByteArray("RIFFfakewavdata"), QStringLiteral("clip.wav"),
                                 QStringLiteral("whisper-1"));
    request.setResponseFormat(QStringLiteral("verbose_json"));

    TranscriptionReply *reply = client.createTranscription(request);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/audio/transcriptions "));
    // The body is a multipart/form-data upload carrying the file and fields.
    QVERIFY(server.requestHeaders().toLower().contains("content-type: multipart/form-data;"));
    const QByteArray body = server.requestBody();
    QVERIFY(body.contains("name=\"file\"; filename=\"clip.wav\""));
    QVERIFY(body.contains("RIFFfakewavdata"));
    QVERIFY(body.contains("name=\"model\""));
    QVERIFY(body.contains("whisper-1"));
    QVERIFY(body.contains("verbose_json"));

    const TranscriptionResponse response = reply->response();
    QCOMPARE(response.text(), QStringLiteral("Hello world"));
    QCOMPARE(response.segments().size(), 1);
    QCOMPARE(response.segments().first().end(), 1.5);
    delete reply;
}

QTEST_MAIN(TestTranscriptionsClient)
#include "tst_transcriptions.moc"
