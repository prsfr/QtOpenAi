// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ContainerFileReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class ContainerFileReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::ContainerFile file;
};

ContainerFileReply::ContainerFileReply(std::function<QNetworkReply *()> requestFactory,
                                       RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new ContainerFileReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::ContainerFile ContainerFileReply::file() const
{
    Q_D(const ContainerFileReply);
    return d->file;
}

bool ContainerFileReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(ContainerFileReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->file = Core::ContainerFile::fromJson(object);
    Q_EMIT finished(d->file);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
