// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VectorStoreFileBatchReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class VectorStoreFileBatchReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::VectorStoreFileBatch batch;
};

VectorStoreFileBatchReply::VectorStoreFileBatchReply(
        std::function<QNetworkReply *()> requestFactory, RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new VectorStoreFileBatchReplyPrivate, std::move(requestFactory),
                    std::move(policy), parent)
{ }

Core::VectorStoreFileBatch VectorStoreFileBatchReply::batch() const
{
    Q_D(const VectorStoreFileBatchReply);
    return d->batch;
}

bool VectorStoreFileBatchReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(VectorStoreFileBatchReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->batch = Core::VectorStoreFileBatch::fromJson(object);
    Q_EMIT finished(d->batch);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
