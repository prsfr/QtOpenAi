// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ConversationItemsReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class ConversationItemsReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::ConversationItemList items;
};

ConversationItemsReply::ConversationItemsReply(std::function<QNetworkReply *()> requestFactory,
                                               RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new ConversationItemsReplyPrivate, std::move(requestFactory),
                    std::move(policy), parent)
{ }

Core::ConversationItemList ConversationItemsReply::items() const
{
    Q_D(const ConversationItemsReply);
    return d->items;
}

Core::ResponseOutputItem ConversationItemsReply::firstItem() const
{
    Q_D(const ConversationItemsReply);
    const auto list = d->items.data;
    return list.isEmpty() ? Core::ResponseOutputItem() : list.first();
}

bool ConversationItemsReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(ConversationItemsReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    if (object.value(QStringLiteral("object")).toString() == QLatin1String("list")) {
        d->items = Core::ConversationItemList::fromJson(object);
    } else {
        // A single item object (get item): surface it as a one-item list.
        d->items.data = {Core::ResponseOutputItem::fromJson(object)};
    }
    Q_EMIT finished(d->items);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
