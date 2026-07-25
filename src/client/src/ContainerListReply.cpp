// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ContainerListReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class ContainerListReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::ContainerList list;
};

ContainerListReply::ContainerListReply(std::function<QNetworkReply *()> requestFactory,
                                       RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new ContainerListReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::ContainerList ContainerListReply::list() const
{
    Q_D(const ContainerListReply);
    return d->list;
}

bool ContainerListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(ContainerListReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->list = Core::ContainerList::fromJson(object);
    Q_EMIT finished(d->list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
