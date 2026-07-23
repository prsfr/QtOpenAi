// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ChatCompletionReply.h"

#include "RestReply_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

class ChatCompletionReplyPrivate
{
public:
    RestReply *engine = nullptr;
    Core::ChatCompletionResponse response;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

ChatCompletionReply::ChatCompletionReply(std::function<QNetworkReply *()> requestFactory,
                                         RetryPolicy policy, QObject *parent)
    : QObject(parent)
    , d_ptr(new ChatCompletionReplyPrivate)
{
    Q_D(ChatCompletionReply);
    d->engine = new RestReply(std::move(requestFactory), std::move(policy), this);

    connect(d->engine, &RestReply::retrying, this, &ChatCompletionReply::retrying);

    connect(d->engine, &RestReply::succeeded, this, [this](const QByteArray &body, int status) {
        Q_D(ChatCompletionReply);
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            d->success = false;
            d->error = ClientError(
                    ClientError::Kind::Parse,
                    QStringLiteral("invalid JSON response: %1").arg(parseError.errorString()),
                    status);
        } else {
            d->response = Core::ChatCompletionResponse::fromJson(doc.object());
            d->success = true;
        }
        d->finished = true;
        if (d->success)
            Q_EMIT finished(d->response);
        else
            Q_EMIT failed(d->error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });

    connect(d->engine, &RestReply::failed, this, [this](const ClientError &error) {
        Q_D(ChatCompletionReply);
        d->finished = true;
        d->success = false;
        d->error = error;
        Q_EMIT failed(error);
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

RateLimit ChatCompletionReply::rateLimit() const
{
    Q_D(const ChatCompletionReply);
    return d->engine->rateLimit();
}

int ChatCompletionReply::retryCount() const
{
    Q_D(const ChatCompletionReply);
    return d->engine->retryCount();
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
    d->engine->abort();
}

} // namespace Client
} // namespace QtOpenAi
