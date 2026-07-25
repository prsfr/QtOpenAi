// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VectorStoreFileContentReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class VectorStoreFileContentReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::VectorStoreFileContentPage page;
};

VectorStoreFileContentReply::VectorStoreFileContentReply(
        std::function<QNetworkReply *()> requestFactory, RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new VectorStoreFileContentReplyPrivate, std::move(requestFactory),
                    std::move(policy), parent)
{ }

Core::VectorStoreFileContentPage VectorStoreFileContentReply::page() const
{
    Q_D(const VectorStoreFileContentReply);
    return d->page;
}

bool VectorStoreFileContentReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(VectorStoreFileContentReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->page = Core::VectorStoreFileContentPage::fromJson(object);
    Q_EMIT finished(d->page);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
