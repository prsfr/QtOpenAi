// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/RealtimeEvent.h>
#include <QtOpenAi/Core/RealtimeSessionConfig.h>
#include <QtOpenAi/Realtime/GlobalRealtime.h>

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QString>
#include <QtCore/QUrl>

namespace QtOpenAi {
namespace Realtime {

class RealtimeConnectionPrivate;

// A live Realtime session over a WebSocket.
//
// Every other endpoint in this library is a request that ends in a reply. This
// one is a channel: it stays open, both sides send events whenever they have
// something to say, and audio flows in both directions while the model is still
// speaking. That is why it is a QObject with signals rather than a reply, and
// why it lives in its own module — QWebSockets is a dependency nothing else
// here needs.
//
// Authenticate with either an API key (server-side) or an ephemeral client
// secret from Client::createRealtimeClientSecret() (anywhere else); both are
// presented the same way, so a program can move from one to the other without
// changing anything but the string.
//
//     RealtimeConnection connection;
//     connection.setApiKey(secret.value());
//     connection.setModel("gpt-realtime");
//     connect(&connection, &RealtimeConnection::audioDelta, this, &Player::play);
//     connection.open();
//     connection.sendText("Hello");
//
// Events sent before the channel finishes opening are queued and flushed on
// connect, so a caller can configure the session on the line after open()
// without waiting for a signal first.
//
// eventReceived() carries every server event, including the ones with no named
// signal — the protocol has some 45 of them and grows. The named signals are
// the handful a caller almost always wants; everything else is reachable
// without waiting for this library to catch up.
class QTOPENAI_REALTIME_EXPORT RealtimeConnection : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QString model READ model WRITE setModel NOTIFY modelChanged)
public:
    explicit RealtimeConnection(QObject *parent = nullptr);
    ~RealtimeConnection() override;

    // The channel endpoint. Defaults to the OpenAI Realtime URL; point it at a
    // compatible server (or a test double) to use another.
    QUrl url() const;
    void setUrl(const QUrl &url);

    // An API key or an ephemeral client secret; sent as `Authorization: Bearer`.
    QString apiKey() const;
    void setApiKey(const QString &apiKey);

    // The Realtime model, appended as the `model` query parameter. Leave empty
    // when the credential already pins one.
    QString model() const;
    void setModel(const QString &model);

    // Extra headers sent with the opening handshake (e.g. provider-specific).
    void setDefaultHeader(const QByteArray &name, const QByteArray &value);

    bool isOpen() const;

    // Open the channel. Asynchronous: connected() reports success, socketError()
    // failure.
    void open();
    // Close it. disconnected() follows.
    void close();

    // Send any client event, including one this library has no helper for.
    void sendEvent(const Core::RealtimeEvent &event);

    // --- The events a session normally sends -------------------------------
    // Reconfigure the running session; only the fields set on `config` travel.
    void updateSession(const Core::RealtimeSessionConfig &config);
    // Append captured audio to the input buffer.
    void sendAudio(const QByteArray &audio);
    // Close the current input turn. Server-side turn detection does this for
    // you; call it when you have turned that off.
    void commitAudio();
    // Drop whatever the input buffer holds.
    void clearAudio();
    // Say something as the user and ask for an answer — the two events a text
    // turn always needs.
    void sendText(const QString &text);
    // Ask for an answer on its own, optionally overriding the session for this
    // one response.
    void createResponse(const QJsonObject &overrides = {});
    // Interrupt the response in progress.
    void cancelResponse();

Q_SIGNALS:
    void connected();
    void disconnected();
    // A transport-level failure (handshake refused, socket dropped). Errors the
    // server reports *on* the channel arrive as errorReceived() instead.
    void socketError(const QString &message);

    // Every server event, in arrival order.
    void eventReceived(const QtOpenAi::Core::RealtimeEvent &event);

    // The session the server resolved, from `session.created`/`session.updated`.
    void sessionCreated(const QtOpenAi::Core::RealtimeSessionConfig &session);
    void sessionUpdated(const QtOpenAi::Core::RealtimeSessionConfig &session);

    // Incremental output. Text and transcript arrive as text; audio arrives
    // decoded, so a player can be fed straight from it.
    void textDelta(const QString &delta);
    void transcriptDelta(const QString &delta);
    void audioDelta(const QByteArray &audio);

    // The model has finished answering (`response.done`).
    void responseFinished(const QtOpenAi::Core::RealtimeEvent &event);
    // An `error` event. The channel stays open.
    void errorReceived(const QtOpenAi::Core::RealtimeEvent &event);

    void urlChanged();
    void apiKeyChanged();
    void modelChanged();

private:
    Q_DECLARE_PRIVATE(RealtimeConnection)
    QScopedPointer<RealtimeConnectionPrivate> d_ptr;
};

} // namespace Realtime
} // namespace QtOpenAi
