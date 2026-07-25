// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ContainerReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class ContainerReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::Container container;
};

ContainerReply::ContainerReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                               QObject *parent)
    : RestReplyBase(*new ContainerReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::Container ContainerReply::container() const
{
    Q_D(const ContainerReply);
    return d->container;
}

bool ContainerReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(ContainerReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->container = Core::Container::fromJson(object);
    Q_EMIT finished(d->container);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
