// SPDX-License-Identifier: MIT
#pragma once

// A QNetworkReply that was never a network reply: it delivers a body handed to
// it at construction, on the next event-loop turn, without opening a socket.
//
// This is what lets an interceptor short-circuit a request (Interceptor::
// beforeRequest returning a response -- the cache is the reason it exists). The
// whole transport stack below RestReply speaks QNetworkReply, so answering a
// request locally means *being* a QNetworkReply rather than special-casing the
// four call sites that issue one. Not part of the public API.

#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <utility>

namespace QtOpenAi {
namespace Client {

class CannedReply : public QNetworkReply
{
    Q_OBJECT
public:
    CannedReply(const QNetworkRequest &request, QByteArray body, int httpStatus,
                QByteArray contentType, QObject *parent = nullptr)
        : QNetworkReply(parent)
        , m_body(std::move(body))
    {
        setRequest(request);
        setUrl(request.url());
        setOperation(QNetworkAccessManager::CustomOperation);
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, httpStatus);
        setHeader(QNetworkRequest::ContentTypeHeader, QString::fromUtf8(contentType));
        setHeader(QNetworkRequest::ContentLengthHeader, qint64(m_body.size()));

        m_buffer.setData(m_body);
        m_buffer.open(QIODevice::ReadOnly);
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);

        // Deferred for the same reason RestReply defers its first attempt: a
        // caller must be able to connect before anything fires.
        QTimer::singleShot(0, this, [this]() {
            setFinished(true);
            Q_EMIT readyRead();
            Q_EMIT finished();
        });
    }

    // Nothing is in flight, so there is nothing to stop.
    void abort() override { }

    qint64 bytesAvailable() const override
    {
        return QNetworkReply::bytesAvailable() + m_buffer.bytesAvailable();
    }

    bool isSequential() const override { return true; }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        const qint64 read = m_buffer.read(data, maxSize);
        // A sequential device reports exhaustion as -1, not as a short read.
        return read == 0 ? -1 : read;
    }

private:
    QByteArray m_body;
    QBuffer m_buffer;
};

} // namespace Client
} // namespace QtOpenAi
