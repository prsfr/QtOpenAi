// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

class TestVideoClient : public QObject
{
    Q_OBJECT
private slots:
    void createPostsJsonAndParsesQueuedJob();
    void createWithReferenceUploadsMultipart();
    void listParsesPage();
    void remixPostsPrompt();
    void editSendsJsonWhenTheSourceIsNamed();
    void editUploadsMultipartWhenTheSourceIsBytes();
    void extendSendsSecondsAndEditDoesNot();
    void namingASourceAndUploadingOneAreExclusive();
    void createsAndFetchesACharacter();
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

    const auto reply = awaited(client.createVideo(request));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLines().first().startsWith("POST /v1/videos "));
    QVERIFY(server.requestBodies().first().contains("\"prompt\":\"a cat surfing\""));
    QVERIFY(server.requestBodies().first().contains("\"seconds\":\"8\""));
    QCOMPARE(reply->job().id(), QStringLiteral("video_1"));
    QCOMPARE(reply->job().status(), VideoStatus::Queued);
    QVERIFY(!reply->job().isTerminal());
}

void TestVideoClient::createWithReferenceUploadsMultipart()
{
    StubServer server(QByteArray(R"({"id":"video_ref","status":"queued","progress":0})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateVideoRequest request(QStringLiteral("extend this"), QStringLiteral("sora-2"));
    request.setInputReference(QStringLiteral("ref.png"), QByteArray("PNGrefbytes"));

    const auto reply = awaited(client.createVideo(request));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLines().first().startsWith("POST /v1/videos "));
    QVERIFY(server.requestHeaders().toLower().contains("content-type: multipart/form-data;"));
    const QByteArray body = server.requestBodies().first();
    QVERIFY(body.contains("name=\"prompt\""));
    QVERIFY(body.contains("name=\"input_reference\"; filename=\"ref.png\""));
    QVERIFY(body.contains("PNGrefbytes"));
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
    const auto reply = awaited(client.listVideos(params));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLines().first().startsWith("GET /v1/videos?"));
    QVERIFY(server.requestLines().first().contains("limit=2"));
    QCOMPARE(reply->list().size(), 2);
    QCOMPARE(reply->list().data.at(0).id(), QStringLiteral("video_1"));
    QCOMPARE(reply->list().data.at(0).status(), VideoStatus::Completed);
}

void TestVideoClient::remixPostsPrompt()
{
    StubServer server(QByteArray(R"({"id":"video_remix","status":"queued","progress":0})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply
            = awaited(client.remixVideo(QStringLiteral("video_1"), QStringLiteral("make it rain")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLines().first().startsWith("POST /v1/videos/video_1/remix "));
    QVERIFY(server.requestBodies().first().contains("\"prompt\":\"make it rain\""));
    QCOMPARE(reply->job().id(), QStringLiteral("video_remix"));
}

void TestVideoClient::editSendsJsonWhenTheSourceIsNamed()
{
    StubServer server(QByteArray(R"({"id":"video_edit","status":"queued","progress":0,
        "remixed_from_video_id":"video_src","prompt":"make it night",
        "expires_at":1799999999})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    VideoSourceRequest request(QStringLiteral("video_src"), QStringLiteral("make it night"));

    const auto reply = awaited(client.editVideo(request));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/videos/edits "));
    // `video` is an object with an id, not a bare string. Flattening it would
    // be accepted by no server and is the mistake worth pinning down.
    const QByteArray body = server.requestBody();
    QVERIFY2(body.contains(R"("video":{"id":"video_src"})"), body.constData());
    QVERIFY(body.contains(R"("prompt":"make it night")"));

    // An edit is a new job that points back at its source rather than a
    // mutation of it.
    QCOMPARE(reply->job().id(), QStringLiteral("video_edit"));
    QCOMPARE(reply->job().remixedFromVideoId(), QStringLiteral("video_src"));
    QCOMPARE(reply->job().prompt(), QStringLiteral("make it night"));
    QCOMPARE(reply->job().expiresAt(), 1799999999);
}

void TestVideoClient::editUploadsMultipartWhenTheSourceIsBytes()
{
    StubServer server(QByteArray(R"({"id":"video_edit","status":"queued","progress":0})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    VideoSourceRequest request;
    request.setPrompt(QStringLiteral("brighter"));
    request.setSourceVideo(QStringLiteral("clip.mp4"), QByteArray("MP4bytes"));

    const auto reply = awaited(client.editVideo(request));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestHeaders().toLower().contains("content-type: multipart/form-data;"));
    const QByteArray body = server.requestBody();
    QVERIFY(body.contains("name=\"video\"; filename=\"clip.mp4\""));
    QVERIFY(body.contains("MP4bytes"));
    QVERIFY(body.contains("name=\"prompt\""));
}

void TestVideoClient::extendSendsSecondsAndEditDoesNot()
{
    StubServer server(QList<StubServer::Response> {
            {R"({"id":"video_ext","status":"queued","progress":0,"seconds":"20"})"},
            {R"({"id":"video_edit","status":"queued","progress":0})"},
    });
    Client client(server.baseUrl(), QStringLiteral("k"));

    VideoSourceRequest request(QStringLiteral("video_src"), QStringLiteral("keep going"));
    request.setSeconds(QStringLiteral("8"));

    const auto extended = awaited(client.extendVideo(request));
    QVERIFY(extended);
    QVERIFY(extended->isSuccess());
    QVERIFY(server.requestLines().at(0).startsWith("POST /v1/videos/extensions "));
    QVERIFY2(server.requestBodies().at(0).contains(R"("seconds":"8")"),
             server.requestBodies().at(0).constData());
    // The reply's seconds is the stitched total, not the 8 that was asked for.
    QCOMPARE(extended->job().seconds(), QStringLiteral("20"));

    // The same request through editVideo() drops `seconds`, because /videos/edits
    // has no such parameter -- one request type, two endpoints, and the caller
    // does not have to remember which fields apply.
    const auto edited = awaited(client.editVideo(request));
    QVERIFY(edited);
    QVERIFY(edited->isSuccess());
    QVERIFY(!server.requestBodies().at(1).contains("seconds"));
}

void TestVideoClient::namingASourceAndUploadingOneAreExclusive()
{
    // A body carrying both is not something the endpoint accepts, and sending
    // the wrong half would render a plausible video of the wrong source -- a
    // failure that looks like success. Setting either clears the other.
    VideoSourceRequest request(QStringLiteral("video_src"), QStringLiteral("p"));
    QVERIFY(!request.hasSourceUpload());

    request.setSourceVideo(QStringLiteral("clip.mp4"), QByteArray("bytes"));
    QVERIFY(request.hasSourceUpload());
    QVERIFY(request.sourceVideoId().isEmpty());
    QVERIFY(!request.toJson(false).contains(QStringLiteral("video")));

    request.setSourceVideoId(QStringLiteral("video_other"));
    QVERIFY(!request.hasSourceUpload());
    QVERIFY(request.sourceVideoData().isEmpty());
    QVERIFY(request.sourceVideoFileName().isEmpty());
}

void TestVideoClient::createsAndFetchesACharacter()
{
    StubServer server(QList<StubServer::Response> {
            {R"({"id":"char_123","name":"Ada","created_at":1730419200})"},
            {R"({"id":"char_123","name":null,"created_at":1730419200})"},
    });
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto created = awaited(client.createVideoCharacter(
            QStringLiteral("Ada"), QStringLiteral("ada.mp4"), QByteArray("MP4bytes")));
    QVERIFY(created);
    QVERIFY(created->isSuccess());
    QVERIFY(server.requestLines().at(0).startsWith("POST /v1/videos/characters "));
    // Always multipart: this endpoint has no JSON variant.
    QVERIFY(server.requestHeaders().toLower().contains("content-type: multipart/form-data;"));
    QVERIFY(server.requestBodies().at(0).contains("name=\"video\"; filename=\"ada.mp4\""));
    QVERIFY(server.requestBodies().at(0).contains("name=\"name\""));
    QCOMPARE(created->character().id(), QStringLiteral("char_123"));
    QCOMPARE(created->character().name(), QStringLiteral("Ada"));
    QCOMPARE(created->character().createdAt(), 1730419200);

    // Both id and name are declared nullable, so a null name is a valid record
    // rather than a decode failure.
    const auto fetched = awaited(client.getVideoCharacter(QStringLiteral("char_123")));
    QVERIFY(fetched);
    QVERIFY(fetched->isSuccess());
    QVERIFY(server.requestLines().at(1).startsWith("GET /v1/videos/characters/char_123 "));
    QVERIFY(fetched->character().name().isEmpty());
    QCOMPARE(fetched->character().id(), QStringLiteral("char_123"));
}

void TestVideoClient::deleteIssuesDeleteVerb()
{
    StubServer server(QByteArray(R"({"id":"video_1","object":"video.deleted","deleted":true})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.deleteVideo(QStringLiteral("video_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLines().first().startsWith("DELETE /v1/videos/video_1 "));
    QCOMPARE(reply->job().id(), QStringLiteral("video_1"));
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

    const auto reply = awaited(client.downloadVideoContent(QStringLiteral("video_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLines().first().startsWith("GET /v1/videos/video_1/content "));
    QCOMPARE(reply->videoData(), video);
    QCOMPARE(reply->contentType(), QByteArray("video/mp4"));
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
