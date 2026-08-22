// SPDX-License-Identifier: MIT
#pragma once

// A minimal offline HTTP stub server shared by the client tests. It listens on
// an ephemeral loopback port, serves a queue of canned responses (repeating the
// last once the queue is exhausted), and records every request (line, headers,
// body). Each request is read in full (Content-Length aware) so JSON and
// multipart bodies are captured intact.
//
// It deliberately has no Q_OBJECT macro: it emits no signals of its own and
// wires everything through member-function pointers / lambdas, so it needs no
// moc even though it lives in a header shared by many test translation units.

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include <memory>
#include <utility>

class StubServer : public QObject
{
public:
    struct Response
    {
        QByteArray body;
        int status = 200;
        QByteArray contentType = "application/json";
        // Extra response headers, for the endpoints that put part of the answer
        // outside the body -- POST /realtime/calls returns the call id in
        // `Location`. Each pair is emitted verbatim as "Name: value".
        QList<QPair<QByteArray, QByteArray>> headers;
    };

    // Serve a queue of responses (FIFO; the last is repeated once exhausted).
    // In the common case each element is just a body: `{{body1}, {body2}, ...}`.
    explicit StubServer(QList<Response> responses, QObject *parent = nullptr)
        : QObject(parent)
        , m_responses(std::move(responses))
    {
        m_server.listen(QHostAddress::LocalHost, 0);
        connect(&m_server, &QTcpServer::newConnection, this, &StubServer::onConnection);
    }
    // 200 with a JSON body.
    explicit StubServer(QByteArray body, QObject *parent = nullptr)
        : StubServer(QList<Response> {{std::move(body), 200, "application/json"}}, parent)
    { }
    // 200 with an explicit Content-Type (binary endpoints).
    StubServer(QByteArray body, QByteArray contentType, QObject *parent = nullptr)
        : StubServer(QList<Response> {{std::move(body), 200, std::move(contentType)}}, parent)
    { }
    // A chosen HTTP status with a JSON body.
    StubServer(int status, QByteArray body, QObject *parent = nullptr)
        : StubServer(QList<Response> {{std::move(body), status, "application/json"}}, parent)
    { }

    QUrl baseUrl() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1/v1").arg(m_server.serverPort()));
    }

    // First recorded request (the common single-request case).
    QByteArray requestLine() const { return m_requestLines.value(0); }
    QByteArray requestBody() const { return m_requestBodies.value(0); }
    QByteArray requestHeaders() const { return m_requestHeaders.value(0); }
    // Every recorded request, in arrival order.
    QList<QByteArray> requestLines() const { return m_requestLines; }
    QList<QByteArray> requestBodies() const { return m_requestBodies; }
    int requestCount() const { return m_requestLines.size(); }

private:
    void onConnection()
    {
        QTcpSocket *socket = m_server.nextPendingConnection();
        auto buffer = std::make_shared<QByteArray>();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer]() {
            *buffer += socket->readAll();
            const int headerEnd = buffer->indexOf("\r\n\r\n");
            if (headerEnd < 0)
                return;
            const QByteArray head = buffer->left(headerEnd);
            int contentLength = 0;
            for (const QByteArray &line : head.split('\n')) {
                const QByteArray l = line.trimmed().toLower();
                if (l.startsWith("content-length:"))
                    contentLength = l.mid(15).trimmed().toInt();
            }
            if (buffer->size() < headerEnd + 4 + contentLength)
                return;

            m_requestLines.append(buffer->left(buffer->indexOf("\r\n")));
            m_requestHeaders.append(head);
            m_requestBodies.append(buffer->mid(headerEnd + 4, contentLength));

            const Response response = nextResponse();
            const QByteArray reason = response.status < 400 ? "OK" : "Error";
            QByteArray extra;
            for (const auto &header : response.headers)
                extra += "\r\n" + header.first + ": " + header.second;
            const QByteArray raw
                    = "HTTP/1.1 " + QByteArray::number(response.status) + " " + reason
                      + "\r\nContent-Type: " + response.contentType
                      + "\r\nContent-Length: " + QByteArray::number(response.body.size()) + extra
                      + "\r\nConnection: close\r\n\r\n" + response.body;
            socket->write(raw);
            socket->flush();
            socket->disconnectFromHost();
        });
    }

    Response nextResponse()
    {
        if (m_index < m_responses.size())
            return m_responses.at(m_index++);
        return m_responses.isEmpty() ? Response {} : m_responses.last();
    }

    QTcpServer m_server;
    QList<Response> m_responses;
    int m_index = 0;
    QList<QByteArray> m_requestLines;
    QList<QByteArray> m_requestHeaders;
    QList<QByteArray> m_requestBodies;
};
