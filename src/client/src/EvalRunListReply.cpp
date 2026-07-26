// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/EvalRunListReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class EvalRunListReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::EvalRunList list;
};

EvalRunListReply::EvalRunListReply(std::function<QNetworkReply *()> requestFactory,
                                   RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new EvalRunListReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::EvalRunList EvalRunListReply::list() const
{
    Q_D(const EvalRunListReply);
    return d->list;
}

bool EvalRunListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(EvalRunListReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->list = Core::EvalRunList::fromJson(object);
    Q_EMIT finished(d->list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
