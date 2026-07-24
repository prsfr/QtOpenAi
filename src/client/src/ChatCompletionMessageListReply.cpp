// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ChatCompletionMessageListReply.h"

namespace QtOpenAi {
namespace Client {

ChatCompletionMessageListReply::ChatCompletionMessageListReply(
        std::function<QNetworkReply *()> requestFactory, RetryPolicy policy, QObject *parent)
    : RestReplyBase(std::move(requestFactory), std::move(policy), parent)
{ }

Core::ChatCompletionMessageList ChatCompletionMessageListReply::list() const { return m_list; }

bool ChatCompletionMessageListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    m_list = Core::ChatCompletionMessageList::fromJson(object);
    Q_EMIT finished(m_list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
