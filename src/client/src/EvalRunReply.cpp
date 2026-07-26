// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/EvalRunReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class EvalRunReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::EvalRun run;
};

EvalRunReply::EvalRunReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                           QObject *parent)
    : RestReplyBase(*new EvalRunReplyPrivate, std::move(requestFactory), std::move(policy), parent)
{ }

Core::EvalRun EvalRunReply::run() const
{
    Q_D(const EvalRunReply);
    return d->run;
}

bool EvalRunReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(EvalRunReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->run = Core::EvalRun::fromJson(object);
    Q_EMIT finished(d->run);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
