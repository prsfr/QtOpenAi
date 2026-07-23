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

private slots:
    void onConnection()
    {
        QTcpSocket *socket = m_server.nextPendingConnection();
        auto buffer = std::make_shared<QByteArray>();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer]() {
            *buffer += socket->readAll();
            if (!buffer->contains("\r\n\r\n"))
                return;
            if (m_requestLine.isEmpty())
                m_requestLine = buffer->left(buffer->indexOf("\r\n"));

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
};

class TestModerationsClient : public QObject
{
    Q_OBJECT
private slots:
    void createPostsAndParsesFlagged();
};

void TestModerationsClient::createPostsAndParsesFlagged()
{
    StubServer server(R"({"id":"modr_1","model":"omni-moderation-latest","results":[
        {"flagged":true,"categories":{"violence":true},
         "category_scores":{"violence":0.91}}]})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    ModerationReply *reply = client.createModeration(ModerationRequest(QStringLiteral("bad text")));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/moderations "));
    const ModerationResult result = reply->response().firstResult();
    QVERIFY(result.flagged());
    QCOMPARE(result.score(QStringLiteral("violence")), 0.91);
    delete reply;
}

QTEST_MAIN(TestModerationsClient)
#include "tst_moderations.moc"
