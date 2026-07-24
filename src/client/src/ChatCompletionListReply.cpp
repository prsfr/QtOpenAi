// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ChatCompletionListReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class ChatCompletionListReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::ChatCompletionList list;
};

ChatCompletionListReply::ChatCompletionListReply(std::function<QNetworkReply *()> requestFactory,
                                                 RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new ChatCompletionListReplyPrivate, std::move(requestFactory),
                    std::move(policy), parent)
{ }

Core::ChatCompletionList ChatCompletionListReply::list() const
{
    Q_D(const ChatCompletionListReply);
    return d->list;
}

bool ChatCompletionListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(ChatCompletionListReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->list = Core::ChatCompletionList::fromJson(object);
    Q_EMIT finished(d->list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
