// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/StubServer.h"

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
    StubServer server(
            QByteArray(R"({"id":"video_1","status":"queued","progress":0,"model":"sora-2"})"));
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
    StubServer server(QByteArray(R"({"id":"video_ref","status":"queued","progress":0})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateVideoRequest request(QStringLiteral("extend this"), QStringLiteral("sora-2"));
    request.setInputReference(QStringLiteral("ref.png"), QByteArray("PNGrefbytes"));

    VideoReply *reply = client.createVideo(request);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLines().first().startsWith("POST /v1/videos "));
    QVERIFY(server.requestHeaders().toLower().contains("content-type: multipart/form-data;"));
    const QByteArray body = server.requestBodies().first();
    QVERIFY(body.contains("name=\"prompt\""));
    QVERIFY(body.contains("name=\"input_reference\"; filename=\"ref.png\""));
    QVERIFY(body.contains("PNGrefbytes"));
    delete reply;
}

void TestVideoClient::listParsesPage()
{
    StubServer server(QByteArray(
            R"({"object":"list","data":[{"id":"video_1","status":"completed"},)"
            R"({"id":"video_2","status":"queued"}],"first_id":"video_1","last_id":"video_2",)"
            R"("has_more":false})"));
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
    StubServer server(QByteArray(R"({"id":"video_remix","status":"queued","progress":0})"));
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
    StubServer server(QByteArray(R"({"id":"video_1","object":"video.deleted","deleted":true})"));
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
    StubServer server(video, QByteArray("video/mp4"));
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
    StubServer server(QList<StubServer::Response> {
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
    StubServer server(QList<StubServer::Response> {
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
