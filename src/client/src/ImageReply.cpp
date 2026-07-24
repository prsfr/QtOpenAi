// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ImageReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class ImageReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::ImageResponse response;
};

ImageReply::ImageReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                       QObject *parent)
    : RestReplyBase(*new ImageReplyPrivate, std::move(requestFactory), std::move(policy), parent)
{ }

Core::ImageResponse ImageReply::response() const
{
    Q_D(const ImageReply);
    return d->response;
}

bool ImageReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(ImageReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->response = Core::ImageResponse::fromJson(object);
    Q_EMIT finished(d->response);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
