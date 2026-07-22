// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ChatCompletionReply.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkReply>

namespace QtOpenAi {
namespace Client {

class ChatCompletionReplyPrivate
{
public:
    QNetworkReply *networkReply = nullptr;
    Core::ChatCompletionResponse response;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

ChatCompletionReply::ChatCompletionReply(QNetworkReply *reply, QObject *parent)
    : QObject(parent)
    , d_ptr(new ChatCompletionReplyPrivate)
{
    Q_D(ChatCompletionReply);
    d->networkReply = reply;
    reply->setParent(this);

    connect(reply, &QNetworkReply::finished, this, [this]() {
        Q_D(ChatCompletionReply);
        d->finished = true;
        QNetworkReply *reply = d->networkReply;
        const QByteArray body = reply->readAll();
        const int status =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        auto fail = [this, d](ClientError::Kind kind, const QString &message, int http) {
            d->success = false;
            d->error = ClientError(kind, message, http);
        };

        // Attempt to parse the (error or success) JSON body once.
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);

        if (reply->error() != QNetworkReply::NoError && status < 400) {
            // Transport failure without an HTTP error body.
            fail(ClientError::Kind::Network, reply->errorString(), status);
        } else if (status >= 400 || reply->error() != QNetworkReply::NoError) {
            // HTTP error: pull structured details from the error envelope.
            QString message = reply->errorString();
            ClientError err(ClientError::Kind::Http, message, status);
            if (doc.isObject()) {
                const QJsonObject errorObject =
                    doc.object().value(QStringLiteral("error")).toObject();
                if (!errorObject.isEmpty()) {
                    message = errorObject.value(QStringLiteral("message")).toString(message);
                    err = ClientError(ClientError::Kind::Http, message, status);
                    err.setType(errorObject.value(QStringLiteral("type")).toString());
                    err.setCode(errorObject.value(QStringLiteral("code")).toString());
                }
            }
            d->success = false;
            d->error = err;
        } else if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            fail(ClientError::Kind::Parse,
                 QStringLiteral("invalid JSON response: %1").arg(parseError.errorString()),
                 status);
        } else {
            d->response = Core::ChatCompletionResponse::fromJson(doc.object());
            d->success = true;
        }

        if (d->success)
            Q_EMIT finished(d->response);
        else
            Q_EMIT failed(d->error);
        Q_EMIT done();

        if (d->autoDelete)
            deleteLater();
    });
}

ChatCompletionReply::~ChatCompletionReply() = default;

bool ChatCompletionReply::isFinished() const
{
    Q_D(const ChatCompletionReply);
    return d->finished;
}

bool ChatCompletionReply::isSuccess() const
{
    Q_D(const ChatCompletionReply);
    return d->success;
}

Core::ChatCompletionResponse ChatCompletionReply::response() const
{
    Q_D(const ChatCompletionReply);
    return d->response;
}

ClientError ChatCompletionReply::error() const
{
    Q_D(const ChatCompletionReply);
    return d->error;
}

void ChatCompletionReply::setAutoDelete(bool enabled)
{
    Q_D(ChatCompletionReply);
    d->autoDelete = enabled;
}

bool ChatCompletionReply::autoDelete() const
{
    Q_D(const ChatCompletionReply);
    return d->autoDelete;
}

void ChatCompletionReply::abort()
{
    Q_D(ChatCompletionReply);
    if (d->networkReply && d->networkReply->isRunning())
        d->networkReply->abort();
}

} // namespace Client
} // namespace QtOpenAi
