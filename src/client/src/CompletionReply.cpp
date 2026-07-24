// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/CompletionReply.h"

namespace QtOpenAi {
namespace Client {

CompletionReply::CompletionReply(std::function<QNetworkReply *()> requestFactory,
                                 RetryPolicy policy, QObject *parent)
    : RestReplyBase(std::move(requestFactory), std::move(policy), parent)
{ }

Core::CompletionResponse CompletionReply::response() const { return m_response; }

bool CompletionReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    m_response = Core::CompletionResponse::fromJson(object);
    Q_EMIT finished(m_response);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
