// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ChatCompletionStreamReply.h"

#include "QtOpenAi/Client/ChatCompletionAccumulator.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkReply>

namespace QtOpenAi {
namespace Client {

class ChatCompletionStreamReplyPrivate
{
public:
    QNetworkReply *networkReply = nullptr;
    ChatCompletionAccumulator accumulator;
    ClientError error;
    QByteArray buffer; // unparsed SSE bytes
    bool finished = false;
    bool success = false;
    bool sawDone = false; // received the terminating [DONE] sentinel
    bool autoDelete = true;
};

ChatCompletionStreamReply::ChatCompletionStreamReply(QNetworkReply *reply, QObject *parent)
    : QObject(parent)
    , d_ptr(new ChatCompletionStreamReplyPrivate)
{
    Q_D(ChatCompletionStreamReply);
    d->networkReply = reply;
    reply->setParent(this);

    connect(reply, &QNetworkReply::readyRead, this, [this]() {
        Q_D(ChatCompletionStreamReply);
        d->buffer += d->networkReply->readAll();
        // SSE events are separated by a blank line. Normalise CRLF first.
        d->buffer.replace("\r\n", "\n");

        int sep;
        while ((sep = d->buffer.indexOf("\n\n")) != -1) {
            const QByteArray event = d->buffer.left(sep);
            d->buffer.remove(0, sep + 2);

            // Concatenate all `data:` field values within this event.
            QByteArray data;
            for (const QByteArray &rawLine : event.split('\n')) {
                QByteArray line = rawLine;
                if (line.startsWith(':')) // comment/heartbeat
                    continue;
                if (line.startsWith("data:")) {
                    line = line.mid(5);
                    if (line.startsWith(' '))
                        line = line.mid(1);
                    data += line;
                }
            }
            if (data.isEmpty())
                continue;
            if (data == "[DONE]") {
                d->sawDone = true;
                continue;
            }

            const QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isObject())
                continue;

            const Core::ChatCompletionChunk chunk
                    = Core::ChatCompletionChunk::fromJson(doc.object());
            d->accumulator.add(chunk);
            Q_EMIT delta(chunk);

            if (!chunk.choices().isEmpty()) {
                const Core::ChoiceDelta first = chunk.choices().first().delta();
                if (first.hasContent() && !first.content().isEmpty())
                    Q_EMIT contentDelta(first.content());
            }
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this]() {
        Q_D(ChatCompletionStreamReply);
        d->finished = true;
        QNetworkReply *reply = d->networkReply;
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError || status >= 400) {
            // Error responses are delivered as a single JSON body, not SSE.
            QString message = reply->errorString();
            ClientError err(status >= 400 ? ClientError::Kind::Http : ClientError::Kind::Network,
                            message, status);
            const QByteArray body = d->buffer + reply->readAll();
            const QJsonDocument doc = QJsonDocument::fromJson(body);
            if (doc.isObject()) {
                const QJsonObject errorObject
                        = doc.object().value(QStringLiteral("error")).toObject();
                if (!errorObject.isEmpty()) {
                    message = errorObject.value(QStringLiteral("message")).toString(message);
                    err = ClientError(ClientError::Kind::Http, message, status);
                    err.setType(errorObject.value(QStringLiteral("type")).toString());
                    err.setCode(errorObject.value(QStringLiteral("code")).toString());
                }
            }
            d->success = false;
            d->error = err;
            Q_EMIT failed(d->error);
        } else {
            d->success = true;
            Q_EMIT finished(d->accumulator.response());
        }

        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

ChatCompletionStreamReply::~ChatCompletionStreamReply() = default;

bool ChatCompletionStreamReply::isFinished() const
{
    Q_D(const ChatCompletionStreamReply);
    return d->finished;
}

bool ChatCompletionStreamReply::isSuccess() const
{
    Q_D(const ChatCompletionStreamReply);
    return d->success;
}

Core::ChatCompletionResponse ChatCompletionStreamReply::response() const
{
    Q_D(const ChatCompletionStreamReply);
    return d->accumulator.response();
}

ClientError ChatCompletionStreamReply::error() const
{
    Q_D(const ChatCompletionStreamReply);
    return d->error;
}

void ChatCompletionStreamReply::setAutoDelete(bool enabled)
{
    Q_D(ChatCompletionStreamReply);
    d->autoDelete = enabled;
}

bool ChatCompletionStreamReply::autoDelete() const
{
    Q_D(const ChatCompletionStreamReply);
    return d->autoDelete;
}

void ChatCompletionStreamReply::abort()
{
    Q_D(ChatCompletionStreamReply);
    if (d->networkReply && d->networkReply->isRunning())
        d->networkReply->abort();
}

} // namespace Client
} // namespace QtOpenAi
