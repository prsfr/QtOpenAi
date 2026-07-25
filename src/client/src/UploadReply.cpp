// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/UploadReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class UploadReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::Upload upload;
};

UploadReply::UploadReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                         QObject *parent)
    : RestReplyBase(*new UploadReplyPrivate, std::move(requestFactory), std::move(policy), parent)
{ }

Core::Upload UploadReply::upload() const
{
    Q_D(const UploadReply);
    return d->upload;
}

bool UploadReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(UploadReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->upload = Core::Upload::fromJson(object);
    Q_EMIT finished(d->upload);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
