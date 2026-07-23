// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ChatCompletionListReply.h"

#include "RestReply_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

class ChatCompletionListReplyPrivate
{
public:
    RestReply *engine = nullptr;
    Core::ChatCompletionList list;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

ChatCompletionListReply::ChatCompletionListReply(std::function<QNetworkReply *()> requestFactory,
                                                 RetryPolicy policy, QObject *parent)
    : QObject(parent)
    , d_ptr(new ChatCompletionListReplyPrivate)
{
    Q_D(ChatCompletionListReply);
    d->engine = new RestReply(std::move(requestFactory), std::move(policy), this);

    connect(d->engine, &RestReply::retrying, this, &ChatCompletionListReply::retrying);

    connect(d->engine, &RestReply::succeeded, this, [this](const QByteArray &body, int status) {
        Q_D(ChatCompletionListReply);
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            d->success = false;
            d->error = ClientError(
                    ClientError::Kind::Parse,
                    QStringLiteral("invalid JSON response: %1").arg(parseError.errorString()),
                    status);
        } else {
            d->list = Core::ChatCompletionList::fromJson(doc.object());
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
        Q_D(ChatCompletionListReply);
        d->finished = true;
        d->success = false;
        d->error = error;
        Q_EMIT failed(error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

ChatCompletionListReply::~ChatCompletionListReply() = default;

bool ChatCompletionListReply::isFinished() const
{
    Q_D(const ChatCompletionListReply);
    return d->finished;
}

bool ChatCompletionListReply::isSuccess() const
{
    Q_D(const ChatCompletionListReply);
    return d->success;
}

Core::ChatCompletionList ChatCompletionListReply::list() const
{
    Q_D(const ChatCompletionListReply);
    return d->list;
}

ClientError ChatCompletionListReply::error() const
{
    Q_D(const ChatCompletionListReply);
    return d->error;
}

RateLimit ChatCompletionListReply::rateLimit() const
{
    Q_D(const ChatCompletionListReply);
    return d->engine->rateLimit();
}

int ChatCompletionListReply::retryCount() const
{
    Q_D(const ChatCompletionListReply);
    return d->engine->retryCount();
}

void ChatCompletionListReply::setAutoDelete(bool enabled)
{
    Q_D(ChatCompletionListReply);
    d->autoDelete = enabled;
}

bool ChatCompletionListReply::autoDelete() const
{
    Q_D(const ChatCompletionListReply);
    return d->autoDelete;
}

void ChatCompletionListReply::abort()
{
    Q_D(ChatCompletionListReply);
    d->engine->abort();
}

} // namespace Client
} // namespace QtOpenAi
