// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

// A stub server that serves a queue of canned responses (FIFO; the last is
// repeated once exhausted), reading each request in full (Content-Length aware,
// so multipart bodies are captured) and recording every request line/body.
class StubServer : public QObject
{
    Q_OBJECT
public:
    struct Response
    {
        QByteArray body;
        QByteArray contentType = "application/json";
    };

    explicit StubServer(QList<Response> responses, QObject *parent = nullptr)
        : QObject(parent)
        , m_responses(std::move(responses))
    {
        m_server.listen(QHostAddress::LocalHost, 0);
        connect(&m_server, &QTcpServer::newConnection, this, &StubServer::onConnection);
    }

    QUrl baseUrl() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1/v1").arg(m_server.serverPort()));
    }

    QList<QByteArray> requestLines() const { return m_requestLines; }
    QList<QByteArray> requestBodies() const { return m_requestBodies; }
    QList<QByteArray> requestHeaders() const { return m_requestHeaders; }

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

            m_requestLines.append(buffer->left(buffer->indexOf("\r\n")));
            m_requestHeaders.append(head);
            m_requestBodies.append(buffer->mid(headerEnd + 4, contentLength));

            const Response response = nextResponse();
            QByteArray raw = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: "
                             + response.contentType
                             + "\r\n"
                               "Content-Length: "
                             + QByteArray::number(response.body.size())
                             + "\r\n"
                               "Connection: close\r\n\r\n"
                             + response.body;
            socket->write(raw);
            socket->flush();
            socket->disconnectFromHost();
        });
    }

private:
    Response nextResponse()
    {
        if (m_index < m_responses.size())
            return m_responses.at(m_index++);
        return m_responses.isEmpty() ? Response {} : m_responses.last();
    }

    QTcpServer m_server;
    QList<Response> m_responses;
    int m_index = 0;
    QList<QByteArray> m_requestLines;
    QList<QByteArray> m_requestHeaders;
    QList<QByteArray> m_requestBodies;
};

class TestVideoClient : public QObject
{
    Q_OBJECT
private slots:
    void createPostsJsonAndParsesQueuedJob();
    void createWithReferenceUploadsMultipart();
    void listParsesPage();
    void remixPostsPrompt();
    void deleteIssuesDeleteVerb();
    void downloadsBinaryContentVerbatim();
    void pollUntilCompleteEmitsProgress();
    void pollSurfacesFailure();
};

void TestVideoClient::createPostsJsonAndParsesQueuedJob()
{
    StubServer server({{R"({"id":"video_1","status":"queued","progress":0,"model":"sora-2"})"}});
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateVideoRequest request(QStringLiteral("a cat surfing"), QStringLiteral("sora-2"));
    request.setSeconds(QStringLiteral("8"));

    VideoReply *reply = client.createVideo(request);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLines().first().startsWith("POST /v1/videos "));
    QVERIFY(server.requestBodies().first().contains("\"prompt\":\"a cat surfing\""));
    QVERIFY(server.requestBodies().first().contains("\"seconds\":\"8\""));
    QCOMPARE(reply->job().id(), QStringLiteral("video_1"));
    QCOMPARE(reply->job().status(), VideoStatus::Queued);
    QVERIFY(!reply->job().isTerminal());
    delete reply;
}

void TestVideoClient::createWithReferenceUploadsMultipart()
{
    StubServer server({{R"({"id":"video_ref","status":"queued","progress":0})"}});
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateVideoRequest request(QStringLiteral("extend this"), QStringLiteral("sora-2"));
    request.setInputReference(QStringLiteral("ref.png"), QByteArray("PNGrefbytes"));

    VideoReply *reply = client.createVideo(request);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLines().first().startsWith("POST /v1/videos "));
    QVERIFY(server.requestHeaders().first().toLower().contains(
            "content-type: multipart/form-data;"));
    const QByteArray body = server.requestBodies().first();
    QVERIFY(body.contains("name=\"prompt\""));
    QVERIFY(body.contains("name=\"input_reference\"; filename=\"ref.png\""));
    QVERIFY(body.contains("PNGrefbytes"));
    delete reply;
}

void TestVideoClient::listParsesPage()
{
    StubServer server(
            {{R"({"object":"list","data":[{"id":"video_1","status":"completed"},)"
              R"({"id":"video_2","status":"queued"}],"first_id":"video_1","last_id":"video_2",)"
              R"("has_more":false})"}});
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.limit = 2;
    VideoListReply *reply = client.listVideos(params);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLines().first().startsWith("GET /v1/videos?"));
    QVERIFY(server.requestLines().first().contains("limit=2"));
    QCOMPARE(reply->list().size(), 2);
    QCOMPARE(reply->list().data.at(0).id(), QStringLiteral("video_1"));
    QCOMPARE(reply->list().data.at(0).status(), VideoStatus::Completed);
    delete reply;
}

void TestVideoClient::remixPostsPrompt()
{
    StubServer server({{R"({"id":"video_remix","status":"queued","progress":0})"}});
    Client client(server.baseUrl(), QStringLiteral("k"));

    VideoReply *reply
            = client.remixVideo(QStringLiteral("video_1"), QStringLiteral("make it rain"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLines().first().startsWith("POST /v1/videos/video_1/remix "));
    QVERIFY(server.requestBodies().first().contains("\"prompt\":\"make it rain\""));
    QCOMPARE(reply->job().id(), QStringLiteral("video_remix"));
    delete reply;
}

void TestVideoClient::deleteIssuesDeleteVerb()
{
    StubServer server({{R"({"id":"video_1","object":"video.deleted","deleted":true})"}});
    Client client(server.baseUrl(), QStringLiteral("k"));

    VideoReply *reply = client.deleteVideo(QStringLiteral("video_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLines().first().startsWith("DELETE /v1/videos/video_1 "));
    QCOMPARE(reply->job().id(), QStringLiteral("video_1"));
    delete reply;
}

void TestVideoClient::downloadsBinaryContentVerbatim()
{
    // Canned "video" including a NUL byte to prove binary-safe handling.
    const QByteArray video = QByteArray("\x00\x00\x00\x18"
                                        "ftypmp42"
                                        "\x00\x01\x02",
                                        15);
    StubServer server({{video, "video/mp4"}});
    Client client(server.baseUrl(), QStringLiteral("k"));

    VideoContentReply *reply = client.downloadVideoContent(QStringLiteral("video_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLines().first().startsWith("GET /v1/videos/video_1/content "));
    QCOMPARE(reply->videoData(), video);
    QCOMPARE(reply->contentType(), QByteArray("video/mp4"));
    delete reply;
}

void TestVideoClient::pollUntilCompleteEmitsProgress()
{
    // The poller issues GET /videos/{id} repeatedly: in_progress, then completed.
    StubServer server({
            {R"({"id":"video_1","status":"in_progress","progress":40})"},
            {R"({"id":"video_1","status":"in_progress","progress":80})"},
            {R"({"id":"video_1","status":"completed","progress":100})"},
    });
    Client client(server.baseUrl(), QStringLiteral("k"));

    VideoPoller *poller = client.pollVideo(QStringLiteral("video_1"), 10);
    poller->setAutoDelete(false);

    QList<int> progressValues;
    connect(poller, &VideoPoller::progressed, this,
            [&progressValues](const VideoJob &job) { progressValues.append(job.progress()); });
    QSignalSpy completedSpy(poller, &VideoPoller::completed);

    poller->start();
    QVERIFY(completedSpy.wait(5000));

    QCOMPARE(completedSpy.count(), 1);
    QVERIFY(poller->isFinished());
    QVERIFY(!poller->isPolling());
    QCOMPARE(poller->job().status(), VideoStatus::Completed);
    QCOMPARE(poller->job().progress(), 100);
    // Saw at least the two in-progress states plus the terminal one.
    QVERIFY(progressValues.size() >= 3);
    QCOMPARE(progressValues.last(), 100);
    // Every poll targets the job endpoint.
    QVERIFY(server.requestLines().first().startsWith("GET /v1/videos/video_1 "));
    delete poller;
}

void TestVideoClient::pollSurfacesFailure()
{
    // A job that reports the `failed` terminal status still completes the poll
    // (rendering finished, unsuccessfully) rather than emitting failed().
    StubServer server({
            {R"({"id":"video_1","status":"in_progress","progress":10})"},
            {R"({"id":"video_1","status":"failed","progress":100,)"
             R"("error":{"code":"x","message":"boom"}})"},
    });
    Client client(server.baseUrl(), QStringLiteral("k"));

    VideoPoller *poller = client.pollVideo(QStringLiteral("video_1"), 10);
    poller->setAutoDelete(false);
    QSignalSpy completedSpy(poller, &VideoPoller::completed);

    poller->start();
    QVERIFY(completedSpy.wait(5000));

    QCOMPARE(completedSpy.count(), 1);
    const VideoJob job = qvariant_cast<VideoJob>(completedSpy.first().first());
    QCOMPARE(job.status(), VideoStatus::Failed);
    QCOMPARE(job.errorMessage(), QStringLiteral("boom"));
    delete poller;
}

QTEST_MAIN(TestVideoClient)
#include "tst_video.moc"
