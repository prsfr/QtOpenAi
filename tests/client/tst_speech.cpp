// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

// A one-shot stub server returning a fixed binary body with a chosen
// Content-Type, and recording the request line + body.
class StubServer : public QObject
{
    Q_OBJECT
public:
    StubServer(QByteArray body, QByteArray contentType, QObject *parent = nullptr)
        : QObject(parent)
        , m_body(std::move(body))
        , m_contentType(std::move(contentType))
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
            const int headerEnd = buffer->indexOf("\r\n\r\n");
            if (headerEnd < 0)
                return;
            if (m_requestLine.isEmpty()) {
                m_requestLine = buffer->left(buffer->indexOf("\r\n"));
                m_requestBody = buffer->mid(headerEnd + 4);
            }

            QByteArray response = "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: "
                                  + m_contentType
                                  + "\r\n"
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
    QByteArray m_contentType;
    QByteArray m_requestLine;
    QByteArray m_requestBody;
};

class TestSpeechClient : public QObject
{
    Q_OBJECT
private slots:
    void surfacesAudioBytesVerbatim();
};

void TestSpeechClient::surfacesAudioBytesVerbatim()
{
    // Canned "audio" including a NUL byte to prove binary-safe handling.
    const QByteArray audio = QByteArray("ID3\x00\x01\x02\x03 fake mp3 bytes", 24);
    StubServer server(audio, "audio/mpeg");
    Client client(server.baseUrl(), QStringLiteral("k"));

    SpeechReply *reply = client.createSpeech(SpeechRequest(
            QStringLiteral("gpt-4o-mini-tts"), QStringLiteral("Hello"), QStringLiteral("alloy")));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/audio/speech "));
    QVERIFY(server.requestBody().contains("\"voice\":\"alloy\""));
    // The client must surface the bytes verbatim, NUL byte and all.
    QCOMPARE(reply->audioData(), audio);
    QCOMPARE(reply->contentType(), QByteArray("audio/mpeg"));
    delete reply;
}

QTEST_MAIN(TestSpeechClient)
#include "tst_speech.moc"
