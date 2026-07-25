// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VectorStoreSearchReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class VectorStoreSearchReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::VectorStoreSearchPage page;
};

VectorStoreSearchReply::VectorStoreSearchReply(std::function<QNetworkReply *()> requestFactory,
                                               RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new VectorStoreSearchReplyPrivate, std::move(requestFactory),
                    std::move(policy), parent)
{ }

Core::VectorStoreSearchPage VectorStoreSearchReply::page() const
{
    Q_D(const VectorStoreSearchReply);
    return d->page;
}

bool VectorStoreSearchReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(VectorStoreSearchReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->page = Core::VectorStoreSearchPage::fromJson(object);
    Q_EMIT finished(d->page);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
