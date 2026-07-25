// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/UploadPartReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class UploadPartReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::UploadPart part;
};

UploadPartReply::UploadPartReply(std::function<QNetworkReply *()> requestFactory,
                                 RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new UploadPartReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::UploadPart UploadPartReply::part() const
{
    Q_D(const UploadPartReply);
    return d->part;
}

bool UploadPartReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(UploadPartReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->part = Core::UploadPart::fromJson(object);
    Q_EMIT finished(d->part);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
