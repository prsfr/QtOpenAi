// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ChatCompletionReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class ChatCompletionReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::ChatCompletionResponse response;
};

ChatCompletionReply::ChatCompletionReply(std::function<QNetworkReply *()> requestFactory,
                                         RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new ChatCompletionReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::ChatCompletionResponse ChatCompletionReply::response() const
{
    Q_D(const ChatCompletionReply);
    return d->response;
}

bool ChatCompletionReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(ChatCompletionReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->response = Core::ChatCompletionResponse::fromJson(object);
    Q_EMIT finished(d->response);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
