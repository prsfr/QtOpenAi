// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ConversationItemsReply.h"

namespace QtOpenAi {
namespace Client {

ConversationItemsReply::ConversationItemsReply(std::function<QNetworkReply *()> requestFactory,
                                               RetryPolicy policy, QObject *parent)
    : RestReplyBase(std::move(requestFactory), std::move(policy), parent)
{ }

Core::ConversationItemList ConversationItemsReply::items() const { return m_items; }

Core::ResponseOutputItem ConversationItemsReply::firstItem() const
{
    const auto list = m_items.items();
    return list.isEmpty() ? Core::ResponseOutputItem() : list.first();
}

bool ConversationItemsReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    if (object.value(QStringLiteral("object")).toString() == QLatin1String("list")) {
        m_items = Core::ConversationItemList::fromJson(object);
    } else {
        // A single item object (get item): surface it as a one-item list.
        m_items.setItems({Core::ResponseOutputItem::fromJson(object)});
    }
    Q_EMIT finished(m_items);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
