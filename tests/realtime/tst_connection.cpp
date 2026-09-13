// SPDX-License-Identifier: MIT
#include <QtOpenAi/Realtime/RealtimeConnection.h>

#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Realtime;

#include "support/StubWebSocketServer.h"

// Loopback coverage for the Realtime WebSocket channel (#25), against a stub
// QWebSocketServer. No audio device, no network beyond loopback.
class TestRealtimeConnection : public QObject
{
    Q_OBJECT
private slots:
    void opensChannelWithModelQueryAndAuthHeader();
    void emitsSessionCreated();
    void routesDeltasToTheirOwnSignals();
    void reportsServerErrors();
    void unknownEventsStillArriveWhole();
    void sendsEventsAsJsonText();
    void queuesEventsSentBeforeTheChannelIsOpen();
    void reportsDisconnection();
    void audioIsNotQueuedForAChannelThatIsNotOpen();
    void closingDiscardsWhatWasQueued();
};

namespace {

// Wait for a signal, failing the calling test with its own line number if it
// never comes -- the WebSocket counterpart of AwaitedReply's timeout.
bool await(QSignalSpy &spy, int count = 1, int timeoutMs = 5000)
{
    return spy.count() >= count || spy.wait(timeoutMs);
}

QJsonObject sessionCreatedEvent()
{
    return QJsonObject {
            {QStringLiteral("type"), QStringLiteral("session.created")},
            {QStringLiteral("event_id"), QStringLiteral("event_1")},
            {QStringLiteral("session"),
             QJsonObject {{QStringLiteral("id"), QStringLiteral("sess_1")},
                          {QStringLiteral("model"), QStringLiteral("gpt-realtime")}}},
    };
}

} // namespace

void TestRealtimeConnection::opensChannelWithModelQueryAndAuthHeader()
{
    StubWebSocketServer server;
    RealtimeConnection connection;
    connection.setUrl(server.url());
    connection.setApiKey(QStringLiteral("ek_123"));
    connection.setModel(QStringLiteral("gpt-realtime"));

    QSignalSpy connected(&connection, &RealtimeConnection::connected);
    connection.open();
    QVERIFY(await(connected));

    QVERIFY(connection.isOpen());
    // The model rides in the query, the credential in the header -- an
    // ephemeral client secret is presented exactly like an API key.
    QCOMPARE(QUrlQuery(server.requestUrl()).queryItemValue(QStringLiteral("model")),
             QStringLiteral("gpt-realtime"));
    QCOMPARE(server.requestHeader("Authorization"), QByteArray("Bearer ek_123"));
}

void TestRealtimeConnection::emitsSessionCreated()
{
    StubWebSocketServer server;
    server.setGreeting({sessionCreatedEvent()});

    RealtimeConnection connection;
    connection.setUrl(server.url());
    QSignalSpy sessionCreated(&connection, &RealtimeConnection::sessionCreated);
    connection.open();
    QVERIFY(await(sessionCreated));

    const auto config = sessionCreated.first().first().value<RealtimeSessionConfig>();
    QCOMPARE(config.id(), QStringLiteral("sess_1"));
    QCOMPARE(config.model(), QStringLiteral("gpt-realtime"));
}

void TestRealtimeConnection::routesDeltasToTheirOwnSignals()
{
    // Text, transcript and audio all arrive in a field called `delta`; only the
    // event type says which is which, and the audio one is base64.
    const QByteArray pcm("\x01\x02\x03\x04", 4);
    StubWebSocketServer server;

    RealtimeConnection connection;
    connection.setUrl(server.url());
    QSignalSpy connected(&connection, &RealtimeConnection::connected);
    QSignalSpy textDelta(&connection, &RealtimeConnection::textDelta);
    QSignalSpy transcriptDelta(&connection, &RealtimeConnection::transcriptDelta);
    QSignalSpy audioDelta(&connection, &RealtimeConnection::audioDelta);
    QSignalSpy responseFinished(&connection, &RealtimeConnection::responseFinished);
    connection.open();
    QVERIFY(await(connected));

    server.send({{QStringLiteral("type"), QStringLiteral("response.output_text.delta")},
                 {QStringLiteral("delta"), QStringLiteral("Hel")}});
    server.send({{QStringLiteral("type"), QStringLiteral("response.output_audio_transcript.delta")},
                 {QStringLiteral("delta"), QStringLiteral("Hello")}});
    server.send({{QStringLiteral("type"), QStringLiteral("response.output_audio.delta")},
                 {QStringLiteral("delta"), QString::fromLatin1(pcm.toBase64())}});
    server.send({{QStringLiteral("type"), QStringLiteral("response.done")},
                 {QStringLiteral("response"),
                  QJsonObject {{QStringLiteral("id"), QStringLiteral("resp_1")}}}});

    QVERIFY(await(responseFinished));
    QCOMPARE(textDelta.count(), 1);
    QCOMPARE(textDelta.first().first().toString(), QStringLiteral("Hel"));
    QCOMPARE(transcriptDelta.count(), 1);
    QCOMPARE(transcriptDelta.first().first().toString(), QStringLiteral("Hello"));
    QCOMPARE(audioDelta.count(), 1);
    QCOMPARE(audioDelta.first().first().toByteArray(), pcm);
}

void TestRealtimeConnection::reportsServerErrors()
{
    StubWebSocketServer server;
    RealtimeConnection connection;
    connection.setUrl(server.url());
    QSignalSpy connected(&connection, &RealtimeConnection::connected);
    QSignalSpy errorReceived(&connection, &RealtimeConnection::errorReceived);
    connection.open();
    QVERIFY(await(connected));

    server.send({{QStringLiteral("type"), QStringLiteral("error")},
                 {QStringLiteral("error"), QJsonObject {{QStringLiteral("message"),
                                                         QStringLiteral("Unknown parameter.")}}}});
    QVERIFY(await(errorReceived));

    // An error is a message on the channel, not a transport failure: the
    // connection stays open and the caller decides what to do.
    QCOMPARE(errorReceived.first().first().value<RealtimeEvent>().errorMessage(),
             QStringLiteral("Unknown parameter."));
    QVERIFY(connection.isOpen());
}

void TestRealtimeConnection::unknownEventsStillArriveWhole()
{
    StubWebSocketServer server;
    RealtimeConnection connection;
    connection.setUrl(server.url());
    QSignalSpy connected(&connection, &RealtimeConnection::connected);
    QSignalSpy eventReceived(&connection, &RealtimeConnection::eventReceived);
    connection.open();
    QVERIFY(await(connected));

    // Nothing here is a closed set: an event the library has no signal for is
    // still delivered, payload intact.
    server.send({{QStringLiteral("type"), QStringLiteral("rate_limits.updated")},
                 {QStringLiteral("rate_limits"),
                  QJsonArray {QJsonObject {{QStringLiteral("remaining"), 99}}}}});
    QVERIFY(await(eventReceived));

    const auto event = eventReceived.first().first().value<RealtimeEvent>();
    QCOMPARE(event.type(), QStringLiteral("rate_limits.updated"));
    QCOMPARE(event.payload()
                     .value(QStringLiteral("rate_limits"))
                     .toArray()
                     .first()
                     .toObject()
                     .value(QStringLiteral("remaining"))
                     .toInt(),
             99);
}

void TestRealtimeConnection::sendsEventsAsJsonText()
{
    const QByteArray pcm("\x01\x02\x03\x04", 4);
    StubWebSocketServer server;

    RealtimeConnection connection;
    connection.setUrl(server.url());
    QSignalSpy connected(&connection, &RealtimeConnection::connected);
    connection.open();
    QVERIFY(await(connected));

    RealtimeSessionConfig config;
    config.setInstructions(QStringLiteral("Speak slowly."));
    connection.updateSession(config);
    connection.sendAudio(pcm);
    connection.commitAudio();
    connection.sendText(QStringLiteral("Hello"));

    QTRY_COMPARE(server.received().size(), 5);
    const QList<QJsonObject> sent = server.received();
    QCOMPARE(sent.at(0).value(QStringLiteral("type")).toString(), QStringLiteral("session.update"));
    QCOMPARE(sent.at(1).value(QStringLiteral("audio")).toString(),
             QString::fromLatin1(pcm.toBase64()));
    QCOMPARE(sent.at(2).value(QStringLiteral("type")).toString(),
             QStringLiteral("input_audio_buffer.commit"));
    // sendText() is the two events a text turn always needs: the item, then the
    // request to answer it.
    QCOMPARE(sent.at(3).value(QStringLiteral("type")).toString(),
             QStringLiteral("conversation.item.create"));
    QCOMPARE(sent.at(4).value(QStringLiteral("type")).toString(),
             QStringLiteral("response.create"));
}

void TestRealtimeConnection::queuesEventsSentBeforeTheChannelIsOpen()
{
    // Opening is asynchronous, so a caller that configures the session straight
    // after open() would otherwise lose those events.
    StubWebSocketServer server;
    RealtimeConnection connection;
    connection.setUrl(server.url());

    connection.open();
    connection.sendText(QStringLiteral("Hello"));

    QTRY_COMPARE(server.received().size(), 2);
    QCOMPARE(server.received().first().value(QStringLiteral("type")).toString(),
             QStringLiteral("conversation.item.create"));
}

void TestRealtimeConnection::audioIsNotQueuedForAChannelThatIsNotOpen()
{
    // Session setup is queued across the asynchronous open; audio is not. Frames
    // captured before the channel exists would arrive seconds later in a session
    // that was not configured when they were recorded, and
    // input_audio_buffer.append carries no timestamp, so nothing downstream
    // could tell. Retaining them is also what made the queue unbounded.
    StubWebSocketServer server;
    RealtimeConnection connection;
    connection.setUrl(server.url());

    connection.open();
    connection.sendAudio(QByteArray(2048, '\0'));
    connection.sendText(QStringLiteral("Hello"));

    // The text event arrives (two events: the item and the response request);
    // the audio does not, so the count is the same as without it.
    QTRY_COMPARE(server.received().size(), 2);
    for (const QJsonObject &event : server.received()) {
        QVERIFY2(event.value(QStringLiteral("type")).toString()
                         != QStringLiteral("input_audio_buffer.append"),
                 "pre-connect audio was delivered into the session");
    }

    // Once the channel is open, audio goes straight out.
    connection.sendAudio(QByteArray(2048, '\0'));
    QTRY_COMPARE(server.received().size(), 3);
    QCOMPARE(server.received().last().value(QStringLiteral("type")).toString(),
             QStringLiteral("input_audio_buffer.append"));
}

void TestRealtimeConnection::closingDiscardsWhatWasQueued()
{
    // flush() runs on `connected`, so anything left queued for a channel that
    // was closed before it opened would be replayed into whatever session opened
    // next.
    StubWebSocketServer server;
    RealtimeConnection connection;
    connection.setUrl(server.url());

    connection.open();
    connection.sendText(QStringLiteral("before the close"));
    connection.close();

    // Reopening must not carry the abandoned run's events with it.
    connection.open();
    connection.sendText(QStringLiteral("after the reopen"));
    QTRY_COMPARE(server.received().size(), 2);
    QCOMPARE(server.received()
                     .first()
                     .value(QStringLiteral("item"))
                     .toObject()
                     .value(QStringLiteral("content"))
                     .toArray()
                     .first()
                     .toObject()
                     .value(QStringLiteral("text"))
                     .toString(),
             QStringLiteral("after the reopen"));
}

void TestRealtimeConnection::reportsDisconnection()
{
    StubWebSocketServer server;
    RealtimeConnection connection;
    connection.setUrl(server.url());
    QSignalSpy connected(&connection, &RealtimeConnection::connected);
    QSignalSpy disconnected(&connection, &RealtimeConnection::disconnected);
    connection.open();
    QVERIFY(await(connected));

    server.closeClient();
    QVERIFY(await(disconnected));
    QVERIFY(!connection.isOpen());
}

QTEST_MAIN(TestRealtimeConnection)
#include "tst_connection.moc"
