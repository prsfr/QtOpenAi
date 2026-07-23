// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ChatCompletionMessageListReply.h"

#include "RestReply_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

class ChatCompletionMessageListReplyPrivate
{
public:
    RestReply *engine = nullptr;
    Core::ChatCompletionMessageList list;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

ChatCompletionMessageListReply::ChatCompletionMessageListReply(
        std::function<QNetworkReply *()> requestFactory, RetryPolicy policy, QObject *parent)
    : QObject(parent)
    , d_ptr(new ChatCompletionMessageListReplyPrivate)
{
    Q_D(ChatCompletionMessageListReply);
    d->engine = new RestReply(std::move(requestFactory), std::move(policy), this);

    connect(d->engine, &RestReply::retrying, this, &ChatCompletionMessageListReply::retrying);

    connect(d->engine, &RestReply::succeeded, this, [this](const QByteArray &body, int status) {
        Q_D(ChatCompletionMessageListReply);
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            d->success = false;
            d->error = ClientError(
                    ClientError::Kind::Parse,
                    QStringLiteral("invalid JSON response: %1").arg(parseError.errorString()),
                    status);
        } else {
            d->list = Core::ChatCompletionMessageList::fromJson(doc.object());
            d->success = true;
        }
        d->finished = true;
        if (d->success)
            Q_EMIT finished(d->list);
        else
            Q_EMIT failed(d->error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });

    connect(d->engine, &RestReply::failed, this, [this](const ClientError &error) {
        Q_D(ChatCompletionMessageListReply);
        d->finished = true;
        d->success = false;
        d->error = error;
        Q_EMIT failed(error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

ChatCompletionMessageListReply::~ChatCompletionMessageListReply() = default;

bool ChatCompletionMessageListReply::isFinished() const
{
    Q_D(const ChatCompletionMessageListReply);
    return d->finished;
}

bool ChatCompletionMessageListReply::isSuccess() const
{
    Q_D(const ChatCompletionMessageListReply);
    return d->success;
}

Core::ChatCompletionMessageList ChatCompletionMessageListReply::list() const
{
    Q_D(const ChatCompletionMessageListReply);
    return d->list;
}

ClientError ChatCompletionMessageListReply::error() const
{
    Q_D(const ChatCompletionMessageListReply);
    return d->error;
}

RateLimit ChatCompletionMessageListReply::rateLimit() const
{
    Q_D(const ChatCompletionMessageListReply);
    return d->engine->rateLimit();
}

int ChatCompletionMessageListReply::retryCount() const
{
    Q_D(const ChatCompletionMessageListReply);
    return d->engine->retryCount();
}

void ChatCompletionMessageListReply::setAutoDelete(bool enabled)
{
    Q_D(ChatCompletionMessageListReply);
    d->autoDelete = enabled;
}

bool ChatCompletionMessageListReply::autoDelete() const
{
    Q_D(const ChatCompletionMessageListReply);
    return d->autoDelete;
}

void ChatCompletionMessageListReply::abort()
{
    Q_D(ChatCompletionMessageListReply);
    d->engine->abort();
}

} // namespace Client
} // namespace QtOpenAi
