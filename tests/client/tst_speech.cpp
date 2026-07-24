// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/StubServer.h"

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
