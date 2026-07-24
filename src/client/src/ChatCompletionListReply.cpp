// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ChatCompletionListReply.h"

namespace QtOpenAi {
namespace Client {

ChatCompletionListReply::ChatCompletionListReply(std::function<QNetworkReply *()> requestFactory,
                                                 RetryPolicy policy, QObject *parent)
    : RestReplyBase(std::move(requestFactory), std::move(policy), parent)
{ }

Core::ChatCompletionList ChatCompletionListReply::list() const { return m_list; }

bool ChatCompletionListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    m_list = Core::ChatCompletionList::fromJson(object);
    Q_EMIT finished(m_list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
