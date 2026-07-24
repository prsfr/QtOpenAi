// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VideoReply.h"

namespace QtOpenAi {
namespace Client {

VideoReply::VideoReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                       QObject *parent)
    : RestReplyBase(std::move(requestFactory), std::move(policy), parent)
{ }

Core::VideoJob VideoReply::job() const { return m_job; }

bool VideoReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    m_job = Core::VideoJob::fromJson(object);
    Q_EMIT finished(m_job);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
