// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ImageReply.h"

namespace QtOpenAi {
namespace Client {

ImageReply::ImageReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                       QObject *parent)
    : RestReplyBase(std::move(requestFactory), std::move(policy), parent)
{ }

Core::ImageResponse ImageReply::response() const { return m_response; }

bool ImageReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    m_response = Core::ImageResponse::fromJson(object);
    Q_EMIT finished(m_response);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
