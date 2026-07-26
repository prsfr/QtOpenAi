// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/EvalListReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class EvalListReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::EvalList list;
};

EvalListReply::EvalListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                             QObject *parent)
    : RestReplyBase(*new EvalListReplyPrivate, std::move(requestFactory), std::move(policy), parent)
{ }

Core::EvalList EvalListReply::list() const
{
    Q_D(const EvalListReply);
    return d->list;
}

bool EvalListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(EvalListReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->list = Core::EvalList::fromJson(object);
    Q_EMIT finished(d->list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
