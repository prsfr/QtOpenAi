// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ChatCompletionReply.h"

namespace QtOpenAi {
namespace Client {

ChatCompletionReply::ChatCompletionReply(std::function<QNetworkReply *()> requestFactory,
                                         RetryPolicy policy, QObject *parent)
    : RestReplyBase(std::move(requestFactory), std::move(policy), parent)
{ }

Core::ChatCompletionResponse ChatCompletionReply::response() const { return m_response; }

bool ChatCompletionReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    m_response = Core::ChatCompletionResponse::fromJson(object);
    Q_EMIT finished(m_response);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
