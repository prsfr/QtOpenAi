// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ChatCompletionStreamReply.h"

#include "HttpSupport_p.h"
#include "QtOpenAi/Client/ChatCompletionAccumulator.h"
#include "SseParser_p.h"

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
    RateLimit rateLimit;
    detail::SseParser parser;
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
        // These streams name their event type inside the payload, so only the
        // framed data is of interest here.
        const QList<detail::SseEvent> events = d->parser.feed(d->networkReply->readAll());
        for (const detail::SseEvent &sse : events) {
            const QByteArray &data = sse.data;
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
        d->rateLimit = detail::parseRateLimit(reply);

        if (reply->error() != QNetworkReply::NoError || status >= 400) {
            // Error responses are delivered as a single JSON body, not SSE.
            QString message = reply->errorString();
            ClientError err(status >= 400 ? ClientError::Kind::Http : ClientError::Kind::Network,
                            message, status);
            const QByteArray body = d->parser.buffered() + reply->readAll();
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

RateLimit ChatCompletionStreamReply::rateLimit() const
{
    Q_D(const ChatCompletionStreamReply);
    return d->rateLimit;
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
