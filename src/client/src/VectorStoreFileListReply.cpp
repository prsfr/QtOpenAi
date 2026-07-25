// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VectorStoreFileListReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class VectorStoreFileListReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::VectorStoreFileList list;
};

VectorStoreFileListReply::VectorStoreFileListReply(std::function<QNetworkReply *()> requestFactory,
                                                   RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new VectorStoreFileListReplyPrivate, std::move(requestFactory),
                    std::move(policy), parent)
{ }

Core::VectorStoreFileList VectorStoreFileListReply::list() const
{
    Q_D(const VectorStoreFileListReply);
    return d->list;
}

bool VectorStoreFileListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(VectorStoreFileListReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->list = Core::VectorStoreFileList::fromJson(object);
    Q_EMIT finished(d->list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
