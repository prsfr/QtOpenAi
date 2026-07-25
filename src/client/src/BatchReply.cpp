// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/BatchReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class BatchReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::Batch batch;
};

BatchReply::BatchReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                       QObject *parent)
    : RestReplyBase(*new BatchReplyPrivate, std::move(requestFactory), std::move(policy), parent)
{ }

Core::Batch BatchReply::batch() const
{
    Q_D(const BatchReply);
    return d->batch;
}

bool BatchReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(BatchReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->batch = Core::Batch::fromJson(object);
    Q_EMIT finished(d->batch);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
