// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ConversationReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class ConversationReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::Conversation conversation;
};

ConversationReply::ConversationReply(std::function<QNetworkReply *()> requestFactory,
                                     RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new ConversationReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::Conversation ConversationReply::conversation() const
{
    Q_D(const ConversationReply);
    return d->conversation;
}

bool ConversationReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(ConversationReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->conversation = Core::Conversation::fromJson(object);
    Q_EMIT finished(d->conversation);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
