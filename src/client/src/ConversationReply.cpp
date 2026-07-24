// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ConversationReply.h"

namespace QtOpenAi {
namespace Client {

ConversationReply::ConversationReply(std::function<QNetworkReply *()> requestFactory,
                                     RetryPolicy policy, QObject *parent)
    : RestReplyBase(std::move(requestFactory), std::move(policy), parent)
{ }

Core::Conversation ConversationReply::conversation() const { return m_conversation; }

bool ConversationReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    m_conversation = Core::Conversation::fromJson(object);
    Q_EMIT finished(m_conversation);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
