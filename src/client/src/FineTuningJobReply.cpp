// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/FineTuningJobReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class FineTuningJobReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::FineTuningJob job;
};

FineTuningJobReply::FineTuningJobReply(std::function<QNetworkReply *()> requestFactory,
                                       RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new FineTuningJobReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::FineTuningJob FineTuningJobReply::job() const
{
    Q_D(const FineTuningJobReply);
    return d->job;
}

bool FineTuningJobReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(FineTuningJobReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->job = Core::FineTuningJob::fromJson(object);
    Q_EMIT finished(d->job);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
