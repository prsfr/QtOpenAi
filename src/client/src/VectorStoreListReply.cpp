// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VectorStoreListReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class VectorStoreListReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::VectorStoreList list;
};

VectorStoreListReply::VectorStoreListReply(std::function<QNetworkReply *()> requestFactory,
                                           RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new VectorStoreListReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::VectorStoreList VectorStoreListReply::list() const
{
    Q_D(const VectorStoreListReply);
    return d->list;
}

bool VectorStoreListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(VectorStoreListReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->list = Core::VectorStoreList::fromJson(object);
    Q_EMIT finished(d->list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
