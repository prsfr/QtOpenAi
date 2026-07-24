// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ResponseReply.h"

namespace QtOpenAi {
namespace Client {

ResponseReply::ResponseReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                             QObject *parent)
    : RestReplyBase(std::move(requestFactory), std::move(policy), parent)
{ }

Core::Response ResponseReply::response() const { return m_response; }

bool ResponseReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    m_response = Core::Response::fromJson(object);
    Q_EMIT finished(m_response);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
