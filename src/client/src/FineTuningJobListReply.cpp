// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/FineTuningJobListReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class FineTuningJobListReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::FineTuningJobList list;
};

FineTuningJobListReply::FineTuningJobListReply(std::function<QNetworkReply *()> requestFactory,
                                               RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new FineTuningJobListReplyPrivate, std::move(requestFactory),
                    std::move(policy), parent)
{ }

Core::FineTuningJobList FineTuningJobListReply::list() const
{
    Q_D(const FineTuningJobListReply);
    return d->list;
}

bool FineTuningJobListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(FineTuningJobListReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->list = Core::FineTuningJobList::fromJson(object);
    Q_EMIT finished(d->list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
