// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/FineTuningPermissionReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class FineTuningPermissionReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::FineTuningCheckpointPermission permission;
};

FineTuningPermissionReply::FineTuningPermissionReply(
        std::function<QNetworkReply *()> requestFactory, RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new FineTuningPermissionReplyPrivate, std::move(requestFactory),
                    std::move(policy), parent)
{ }

Core::FineTuningCheckpointPermission FineTuningPermissionReply::permission() const
{
    Q_D(const FineTuningPermissionReply);
    return d->permission;
}

bool FineTuningPermissionReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(FineTuningPermissionReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->permission = Core::FineTuningCheckpointPermission::fromJson(object);
    Q_EMIT finished(d->permission);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
