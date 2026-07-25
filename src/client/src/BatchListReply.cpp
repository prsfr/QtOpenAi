// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/BatchListReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class BatchListReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::BatchList list;
};

BatchListReply::BatchListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                               QObject *parent)
    : RestReplyBase(*new BatchListReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::BatchList BatchListReply::list() const
{
    Q_D(const BatchListReply);
    return d->list;
}

bool BatchListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(BatchListReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->list = Core::BatchList::fromJson(object);
    Q_EMIT finished(d->list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
