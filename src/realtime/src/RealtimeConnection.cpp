// SPDX-License-Identifier: MIT
#include "QtOpenAi/Realtime/RealtimeConnection.h"

#include <QtCore/QHash>
#include <QtCore/QJsonDocument>
#include <QtCore/QList>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QNetworkRequest>
#include <QtWebSockets/QWebSocket>

namespace QtOpenAi {
namespace Realtime {

namespace {
constexpr auto kDefaultUrl = "wss://api.openai.com/v1/realtime";

// The server events that get a signal of their own, and which one. Every other
// event still reaches the caller through eventReceived(); this table only says
// which few are common enough to be worth naming. Keeping it a table rather than
// an if-chain is what makes adding one a single line.
enum class Named {
    SessionCreated,
    SessionUpdated,
    TextDelta,
    TranscriptDelta,
    AudioDelta,
    ResponseFinished,
    Error,
};

struct NamedEvent
{
    const char *type;
    Named signal;
};

constexpr NamedEvent kNamedEvents[] = {
        {"session.created", Named::SessionCreated},
        {"session.updated", Named::SessionUpdated},
        {"response.output_text.delta", Named::TextDelta},
        {"response.output_audio_transcript.delta", Named::TranscriptDelta},
        {"response.output_audio.delta", Named::AudioDelta},
        {"response.done", Named::ResponseFinished},
        {"error", Named::Error},
};

} // namespace

class RealtimeConnectionPrivate
{
public:
    explicit RealtimeConnectionPrivate(RealtimeConnection *connection)
        : q(connection)
    { }

    // Deliver one decoded server event: always through eventReceived(), and
    // through its own signal when it has one.
    void dispatch(const Core::RealtimeEvent &event);
    // Send now, or hold until the channel opens.
    void enqueue(const Core::RealtimeEvent &event);
    void flush();

    RealtimeConnection *q;
    QUrl url = QUrl(QLatin1String(kDefaultUrl));
    QString apiKey;
    QString model;
    QHash<QByteArray, QByteArray> defaultHeaders;
    QWebSocket socket;
    QList<Core::RealtimeEvent> pending;
};

void RealtimeConnectionPrivate::dispatch(const Core::RealtimeEvent &event)
{
    Q_EMIT q->eventReceived(event);

    for (const NamedEvent &named : kNamedEvents) {
        if (event.type() != QLatin1String(named.type))
            continue;
        switch (named.signal) {
        case Named::SessionCreated:
            Q_EMIT q->sessionCreated(event.session());
            return;
        case Named::SessionUpdated:
            Q_EMIT q->sessionUpdated(event.session());
            return;
        case Named::TextDelta:
            Q_EMIT q->textDelta(event.delta());
            return;
        case Named::TranscriptDelta:
            Q_EMIT q->transcriptDelta(event.delta());
            return;
        case Named::AudioDelta:
            Q_EMIT q->audioDelta(event.audioDelta());
            return;
        case Named::ResponseFinished:
            Q_EMIT q->responseFinished(event);
            return;
        case Named::Error:
            Q_EMIT q->errorReceived(event);
            return;
        }
    }
}

void RealtimeConnectionPrivate::enqueue(const Core::RealtimeEvent &event)
{
    if (socket.state() != QAbstractSocket::ConnectedState) {
        // Opening is asynchronous, so a caller that configures the session on
        // the line after open() would otherwise lose those events.
        pending.append(event);
        return;
    }
    socket.sendTextMessage(
            QString::fromUtf8(QJsonDocument(event.toJson()).toJson(QJsonDocument::Compact)));
}

void RealtimeConnectionPrivate::flush()
{
    const QList<Core::RealtimeEvent> queued = std::move(pending);
    pending.clear();
    for (const Core::RealtimeEvent &event : queued)
        enqueue(event);
}

RealtimeConnection::RealtimeConnection(QObject *parent)
    : QObject(parent)
    , d_ptr(new RealtimeConnectionPrivate(this))
{
    Q_D(RealtimeConnection);
    connect(&d->socket, &QWebSocket::connected, this, [this, d] {
        d->flush();
        Q_EMIT connected();
    });
    connect(&d->socket, &QWebSocket::disconnected, this, &RealtimeConnection::disconnected);
    connect(&d->socket, &QWebSocket::textMessageReceived, this, [d](const QString &message) {
        d->dispatch(
                Core::RealtimeEvent::fromJson(QJsonDocument::fromJson(message.toUtf8()).object()));
    });
    // QWebSocket renamed its failure signal in 6.5; the library supports 6.4+,
    // so both spellings are wired here rather than raising the floor.
    const auto reportSocketError = [this, d](QAbstractSocket::SocketError) {
        Q_EMIT socketError(d->socket.errorString());
    };
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(&d->socket, &QWebSocket::errorOccurred, this, reportSocketError);
#else
    connect(&d->socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this,
            reportSocketError);
#endif
}

RealtimeConnection::~RealtimeConnection() = default;

QUrl RealtimeConnection::url() const
{
    Q_D(const RealtimeConnection);
    return d->url;
}

void RealtimeConnection::setUrl(const QUrl &url)
{
    Q_D(RealtimeConnection);
    if (d->url == url)
        return;
    d->url = url;
    Q_EMIT urlChanged();
}

QString RealtimeConnection::apiKey() const
{
    Q_D(const RealtimeConnection);
    return d->apiKey;
}

void RealtimeConnection::setApiKey(const QString &apiKey)
{
    Q_D(RealtimeConnection);
    if (d->apiKey == apiKey)
        return;
    d->apiKey = apiKey;
    Q_EMIT apiKeyChanged();
}

QString RealtimeConnection::model() const
{
    Q_D(const RealtimeConnection);
    return d->model;
}

void RealtimeConnection::setModel(const QString &model)
{
    Q_D(RealtimeConnection);
    if (d->model == model)
        return;
    d->model = model;
    Q_EMIT modelChanged();
}

void RealtimeConnection::setDefaultHeader(const QByteArray &name, const QByteArray &value)
{
    Q_D(RealtimeConnection);
    d->defaultHeaders.insert(name, value);
}

bool RealtimeConnection::isOpen() const
{
    Q_D(const RealtimeConnection);
    return d->socket.state() == QAbstractSocket::ConnectedState;
}

void RealtimeConnection::open()
{
    Q_D(RealtimeConnection);
    QUrl url = d->url;
    if (!d->model.isEmpty()) {
        QUrlQuery query(url);
        query.addQueryItem(QStringLiteral("model"), d->model);
        url.setQuery(query);
    }

    QNetworkRequest request(url);
    // An API key and an ephemeral client secret are presented identically, so
    // moving a program from server-side to browser-side changes only the value.
    if (!d->apiKey.isEmpty())
        request.setRawHeader("Authorization", QByteArray("Bearer ") + d->apiKey.toUtf8());
    for (auto it = d->defaultHeaders.constBegin(); it != d->defaultHeaders.constEnd(); ++it)
        request.setRawHeader(it.key(), it.value());

    d->socket.open(request);
}

void RealtimeConnection::close()
{
    Q_D(RealtimeConnection);
    d->socket.close();
}

void RealtimeConnection::sendEvent(const Core::RealtimeEvent &event)
{
    Q_D(RealtimeConnection);
    d->enqueue(event);
}

void RealtimeConnection::updateSession(const Core::RealtimeSessionConfig &config)
{
    sendEvent(Core::RealtimeEvent::sessionUpdate(config));
}

void RealtimeConnection::sendAudio(const QByteArray &audio)
{
    sendEvent(Core::RealtimeEvent::appendInputAudio(audio));
}

void RealtimeConnection::commitAudio() { sendEvent(Core::RealtimeEvent::commitInputAudio()); }

void RealtimeConnection::clearAudio() { sendEvent(Core::RealtimeEvent::clearInputAudio()); }

void RealtimeConnection::sendText(const QString &text)
{
    sendEvent(Core::RealtimeEvent::userMessage(text));
    sendEvent(Core::RealtimeEvent::createResponse());
}

void RealtimeConnection::createResponse(const QJsonObject &overrides)
{
    sendEvent(Core::RealtimeEvent::createResponse(overrides));
}

void RealtimeConnection::cancelResponse() { sendEvent(Core::RealtimeEvent::cancelResponse()); }

} // namespace Realtime
} // namespace QtOpenAi
