// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VideoListReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class VideoListReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::VideoList list;
};

VideoListReply::VideoListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                               QObject *parent)
    : RestReplyBase(*new VideoListReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::VideoList VideoListReply::list() const
{
    Q_D(const VideoListReply);
    return d->list;
}

bool VideoListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(VideoListReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->list = Core::VideoList::fromJson(object);
    Q_EMIT finished(d->list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
