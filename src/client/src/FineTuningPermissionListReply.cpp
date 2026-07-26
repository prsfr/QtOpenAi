// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/FineTuningPermissionListReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class FineTuningPermissionListReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::FineTuningCheckpointPermissionList list;
};

FineTuningPermissionListReply::FineTuningPermissionListReply(
        std::function<QNetworkReply *()> requestFactory, RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new FineTuningPermissionListReplyPrivate, std::move(requestFactory),
                    std::move(policy), parent)
{ }

Core::FineTuningCheckpointPermissionList FineTuningPermissionListReply::list() const
{
    Q_D(const FineTuningPermissionListReply);
    return d->list;
}

bool FineTuningPermissionListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(FineTuningPermissionListReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->list = Core::FineTuningCheckpointPermissionList::fromJson(object);
    Q_EMIT finished(d->list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
