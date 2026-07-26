// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/EvalRunOutputItemReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class EvalRunOutputItemReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::EvalRunOutputItem item;
};

EvalRunOutputItemReply::EvalRunOutputItemReply(std::function<QNetworkReply *()> requestFactory,
                                               RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new EvalRunOutputItemReplyPrivate, std::move(requestFactory),
                    std::move(policy), parent)
{ }

Core::EvalRunOutputItem EvalRunOutputItemReply::item() const
{
    Q_D(const EvalRunOutputItemReply);
    return d->item;
}

bool EvalRunOutputItemReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(EvalRunOutputItemReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->item = Core::EvalRunOutputItem::fromJson(object);
    Q_EMIT finished(d->item);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
