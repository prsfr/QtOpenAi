// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ChatCompletionMessageListReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class ChatCompletionMessageListReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::ChatCompletionMessageList list;
};

ChatCompletionMessageListReply::ChatCompletionMessageListReply(
        std::function<QNetworkReply *()> requestFactory, RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new ChatCompletionMessageListReplyPrivate, std::move(requestFactory),
                    std::move(policy), parent)
{ }

Core::ChatCompletionMessageList ChatCompletionMessageListReply::list() const
{
    Q_D(const ChatCompletionMessageListReply);
    return d->list;
}

bool ChatCompletionMessageListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(ChatCompletionMessageListReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->list = Core::ChatCompletionMessageList::fromJson(object);
    Q_EMIT finished(d->list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
