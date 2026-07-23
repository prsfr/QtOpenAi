// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ConversationReply.h"

#include "RestReply_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

class ConversationReplyPrivate
{
public:
    RestReply *engine = nullptr;
    Core::Conversation conversation;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

ConversationReply::ConversationReply(std::function<QNetworkReply *()> requestFactory,
                                     RetryPolicy policy, QObject *parent)
    : QObject(parent)
    , d_ptr(new ConversationReplyPrivate)
{
    Q_D(ConversationReply);
    d->engine = new RestReply(std::move(requestFactory), std::move(policy), this);

    connect(d->engine, &RestReply::retrying, this, &ConversationReply::retrying);

    connect(d->engine, &RestReply::succeeded, this, [this](const QByteArray &body, int status) {
        Q_D(ConversationReply);
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            d->success = false;
            d->error = ClientError(
                    ClientError::Kind::Parse,
                    QStringLiteral("invalid JSON response: %1").arg(parseError.errorString()),
                    status);
        } else {
            d->conversation = Core::Conversation::fromJson(doc.object());
            d->success = true;
        }
        d->finished = true;
        if (d->success)
            Q_EMIT finished(d->conversation);
        else
            Q_EMIT failed(d->error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });

    connect(d->engine, &RestReply::failed, this, [this](const ClientError &error) {
        Q_D(ConversationReply);
        d->finished = true;
        d->success = false;
        d->error = error;
        Q_EMIT failed(error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

ConversationReply::~ConversationReply() = default;

bool ConversationReply::isFinished() const
{
    Q_D(const ConversationReply);
    return d->finished;
}

bool ConversationReply::isSuccess() const
{
    Q_D(const ConversationReply);
    return d->success;
}

Core::Conversation ConversationReply::conversation() const
{
    Q_D(const ConversationReply);
    return d->conversation;
}

ClientError ConversationReply::error() const
{
    Q_D(const ConversationReply);
    return d->error;
}

RateLimit ConversationReply::rateLimit() const
{
    Q_D(const ConversationReply);
    return d->engine->rateLimit();
}

int ConversationReply::retryCount() const
{
    Q_D(const ConversationReply);
    return d->engine->retryCount();
}

void ConversationReply::setAutoDelete(bool enabled)
{
    Q_D(ConversationReply);
    d->autoDelete = enabled;
}

bool ConversationReply::autoDelete() const
{
    Q_D(const ConversationReply);
    return d->autoDelete;
}

void ConversationReply::abort()
{
    Q_D(ConversationReply);
    d->engine->abort();
}

} // namespace Client
} // namespace QtOpenAi
