// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/FineTuningEventListReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class FineTuningEventListReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::FineTuningJobEventList list;
};

FineTuningEventListReply::FineTuningEventListReply(std::function<QNetworkReply *()> requestFactory,
                                                   RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new FineTuningEventListReplyPrivate, std::move(requestFactory),
                    std::move(policy), parent)
{ }

Core::FineTuningJobEventList FineTuningEventListReply::list() const
{
    Q_D(const FineTuningEventListReply);
    return d->list;
}

bool FineTuningEventListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(FineTuningEventListReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->list = Core::FineTuningJobEventList::fromJson(object);
    Q_EMIT finished(d->list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
