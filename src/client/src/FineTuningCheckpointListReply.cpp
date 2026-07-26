// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/FineTuningCheckpointListReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class FineTuningCheckpointListReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::FineTuningJobCheckpointList list;
};

FineTuningCheckpointListReply::FineTuningCheckpointListReply(
        std::function<QNetworkReply *()> requestFactory, RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new FineTuningCheckpointListReplyPrivate, std::move(requestFactory),
                    std::move(policy), parent)
{ }

Core::FineTuningJobCheckpointList FineTuningCheckpointListReply::list() const
{
    Q_D(const FineTuningCheckpointListReply);
    return d->list;
}

bool FineTuningCheckpointListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(FineTuningCheckpointListReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->list = Core::FineTuningJobCheckpointList::fromJson(object);
    Q_EMIT finished(d->list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
