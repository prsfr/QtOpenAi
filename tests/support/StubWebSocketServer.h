// SPDX-License-Identifier: MIT
#pragma once

// A minimal offline WebSocket stub server for the Realtime tests. It listens on
// an ephemeral loopback port, records every text message it receives, and can
// push canned server events back to the connected client.
//
// It is the QWebSocket counterpart of StubServer and follows the same rules: no
// Q_OBJECT macro (it emits no signals of its own and wires everything through
// member-function pointers / lambdas, so it needs no moc even though it lives in
// a header shared by several test translation units), and no network access
// beyond loopback.

#include <QtCore/QByteArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtNetwork/QHostAddress>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>

#include <utility>

class StubWebSocketServer : public QObject
{
public:
    explicit StubWebSocketServer(QObject *parent = nullptr)
        : QObject(parent)
        , m_server(QStringLiteral("qtopenai-stub"), QWebSocketServer::NonSecureMode)
    {
        m_server.listen(QHostAddress::LocalHost, 0);
        connect(&m_server, &QWebSocketServer::newConnection, this,
                &StubWebSocketServer::onConnection);
    }

    ~StubWebSocketServer() override { m_server.close(); }

    QUrl url() const
    {
        return QUrl(QStringLiteral("ws://127.0.0.1:%1/v1/realtime").arg(m_server.serverPort()));
    }

    // Events to push the moment a client connects, in order.
    void setGreeting(QList<QJsonObject> events) { m_greeting = std::move(events); }

    // Push one more event to the connected client.
    void send(const QJsonObject &event)
    {
        if (m_socket)
            m_socket->sendTextMessage(QString::fromUtf8(QJsonDocument(event).toJson()));
    }

    // Everything the client sent, decoded, in arrival order.
    QList<QJsonObject> received() const { return m_received; }
    // The handshake request the client opened the connection with.
    QUrl requestUrl() const { return m_requestUrl; }
    QByteArray requestHeader(const QByteArray &name) const { return m_requestHeaders.value(name); }
    bool hasClient() const { return m_socket != nullptr; }

    // Drop the connection from the server side.
    void closeClient()
    {
        if (m_socket)
            m_socket->close();
    }

private:
    void onConnection()
    {
        m_socket = m_server.nextPendingConnection();
        const QNetworkRequest request = m_socket->request();
        m_requestUrl = request.url();
        for (const QByteArray &name : request.rawHeaderList())
            m_requestHeaders.insert(name, request.rawHeader(name));

        connect(m_socket, &QWebSocket::textMessageReceived, this, [this](const QString &message) {
            m_received.append(QJsonDocument::fromJson(message.toUtf8()).object());
        });
        for (const QJsonObject &event : m_greeting)
            send(event);
    }

    QWebSocketServer m_server;
    QWebSocket *m_socket = nullptr;
    QList<QJsonObject> m_greeting;
    QList<QJsonObject> m_received;
    QUrl m_requestUrl;
    QHash<QByteArray, QByteArray> m_requestHeaders;
};
