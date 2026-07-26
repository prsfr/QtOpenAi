// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/EvalRunOutputItemListReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class EvalRunOutputItemListReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::EvalRunOutputItemList list;
};

EvalRunOutputItemListReply::EvalRunOutputItemListReply(
        std::function<QNetworkReply *()> requestFactory, RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new EvalRunOutputItemListReplyPrivate, std::move(requestFactory),
                    std::move(policy), parent)
{ }

Core::EvalRunOutputItemList EvalRunOutputItemListReply::list() const
{
    Q_D(const EvalRunOutputItemListReply);
    return d->list;
}

bool EvalRunOutputItemListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(EvalRunOutputItemListReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->list = Core::EvalRunOutputItemList::fromJson(object);
    Q_EMIT finished(d->list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
