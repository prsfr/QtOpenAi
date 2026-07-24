// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/StubServer.h"

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
