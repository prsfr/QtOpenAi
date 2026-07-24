// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VideoListReply.h"

namespace QtOpenAi {
namespace Client {

VideoListReply::VideoListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                               QObject *parent)
    : RestReplyBase(std::move(requestFactory), std::move(policy), parent)
{ }

Core::VideoList VideoListReply::list() const { return m_list; }

bool VideoListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    m_list = Core::VideoList::fromJson(object);
    Q_EMIT finished(m_list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
