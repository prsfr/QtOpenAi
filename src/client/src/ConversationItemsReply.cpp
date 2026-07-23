// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ConversationItemsReply.h"

#include "RestReply_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

class ConversationItemsReplyPrivate
{
public:
    RestReply *engine = nullptr;
    Core::ConversationItemList items;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

ConversationItemsReply::ConversationItemsReply(std::function<QNetworkReply *()> requestFactory,
                                               RetryPolicy policy, QObject *parent)
    : QObject(parent)
    , d_ptr(new ConversationItemsReplyPrivate)
{
    Q_D(ConversationItemsReply);
    d->engine = new RestReply(std::move(requestFactory), std::move(policy), this);

    connect(d->engine, &RestReply::retrying, this, &ConversationItemsReply::retrying);

    connect(d->engine, &RestReply::succeeded, this, [this](const QByteArray &body, int status) {
        Q_D(ConversationItemsReply);
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            d->success = false;
            d->error = ClientError(
                    ClientError::Kind::Parse,
                    QStringLiteral("invalid JSON response: %1").arg(parseError.errorString()),
                    status);
        } else {
            const QJsonObject object = doc.object();
            if (object.value(QStringLiteral("object")).toString() == QLatin1String("list")) {
                d->items = Core::ConversationItemList::fromJson(object);
            } else {
                // A single item object (get item): surface it as a one-item list.
                d->items.setItems({Core::ResponseOutputItem::fromJson(object)});
            }
            d->success = true;
        }
        d->finished = true;
        if (d->success)
            Q_EMIT finished(d->items);
        else
            Q_EMIT failed(d->error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });

    connect(d->engine, &RestReply::failed, this, [this](const ClientError &error) {
        Q_D(ConversationItemsReply);
        d->finished = true;
        d->success = false;
        d->error = error;
        Q_EMIT failed(error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

ConversationItemsReply::~ConversationItemsReply() = default;

bool ConversationItemsReply::isFinished() const
{
    Q_D(const ConversationItemsReply);
    return d->finished;
}

bool ConversationItemsReply::isSuccess() const
{
    Q_D(const ConversationItemsReply);
    return d->success;
}

Core::ConversationItemList ConversationItemsReply::items() const
{
    Q_D(const ConversationItemsReply);
    return d->items;
}

Core::ResponseOutputItem ConversationItemsReply::firstItem() const
{
    Q_D(const ConversationItemsReply);
    const QList<Core::ResponseOutputItem> list = d->items.items();
    return list.isEmpty() ? Core::ResponseOutputItem() : list.first();
}

ClientError ConversationItemsReply::error() const
{
    Q_D(const ConversationItemsReply);
    return d->error;
}

RateLimit ConversationItemsReply::rateLimit() const
{
    Q_D(const ConversationItemsReply);
    return d->engine->rateLimit();
}

int ConversationItemsReply::retryCount() const
{
    Q_D(const ConversationItemsReply);
    return d->engine->retryCount();
}

void ConversationItemsReply::setAutoDelete(bool enabled)
{
    Q_D(ConversationItemsReply);
    d->autoDelete = enabled;
}

bool ConversationItemsReply::autoDelete() const
{
    Q_D(const ConversationItemsReply);
    return d->autoDelete;
}

void ConversationItemsReply::abort()
{
    Q_D(ConversationItemsReply);
    d->engine->abort();
}

} // namespace Client
} // namespace QtOpenAi
