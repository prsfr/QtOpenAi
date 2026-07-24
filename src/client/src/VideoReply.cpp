// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VideoReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class VideoReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::VideoJob job;
};

VideoReply::VideoReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                       QObject *parent)
    : RestReplyBase(*new VideoReplyPrivate, std::move(requestFactory), std::move(policy), parent)
{ }

Core::VideoJob VideoReply::job() const
{
    Q_D(const VideoReply);
    return d->job;
}

bool VideoReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(VideoReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->job = Core::VideoJob::fromJson(object);
    Q_EMIT finished(d->job);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
