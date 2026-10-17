// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

class TestSpeechClient : public QObject
{
    Q_OBJECT
private slots:
    void surfacesAudioBytesVerbatim();
};

void TestSpeechClient::surfacesAudioBytesVerbatim()
{
    // Canned "audio" including a NUL byte to prove binary-safe handling. The
    // literal's own length, not a hand count: 24 read past its end (#206).
    const QByteArray audio = QByteArrayLiteral("ID3\x00\x01\x02\x03 fake mp3 bytes");
    StubServer server(audio, "audio/mpeg");
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.createSpeech(SpeechRequest(
            QStringLiteral("gpt-4o-mini-tts"), QStringLiteral("Hello"), QStringLiteral("alloy"))));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/audio/speech "));
    QVERIFY(server.requestBody().contains("\"voice\":\"alloy\""));
    // The client must surface the bytes verbatim, NUL byte and all.
    QCOMPARE(reply->audioData(), audio);
    QCOMPARE(reply->contentType(), QByteArray("audio/mpeg"));
}

QTEST_MAIN(TestSpeechClient)
#include "tst_speech.moc"
