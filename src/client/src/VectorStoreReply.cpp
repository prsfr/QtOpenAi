// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VectorStoreReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class VectorStoreReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::VectorStore store;
};

VectorStoreReply::VectorStoreReply(std::function<QNetworkReply *()> requestFactory,
                                   RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new VectorStoreReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::VectorStore VectorStoreReply::store() const
{
    Q_D(const VectorStoreReply);
    return d->store;
}

bool VectorStoreReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(VectorStoreReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->store = Core::VectorStore::fromJson(object);
    Q_EMIT finished(d->store);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
