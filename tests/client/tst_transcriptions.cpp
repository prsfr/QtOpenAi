// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/StubServer.h"

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
