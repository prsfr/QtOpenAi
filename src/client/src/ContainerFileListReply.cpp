// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ContainerFileListReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class ContainerFileListReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::ContainerFileList list;
};

ContainerFileListReply::ContainerFileListReply(std::function<QNetworkReply *()> requestFactory,
                                               RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new ContainerFileListReplyPrivate, std::move(requestFactory),
                    std::move(policy), parent)
{ }

Core::ContainerFileList ContainerFileListReply::list() const
{
    Q_D(const ContainerFileListReply);
    return d->list;
}

bool ContainerFileListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(ContainerFileListReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->list = Core::ContainerFileList::fromJson(object);
    Q_EMIT finished(d->list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
