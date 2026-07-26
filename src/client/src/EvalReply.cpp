// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/EvalReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class EvalReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::Eval eval;
};

EvalReply::EvalReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                     QObject *parent)
    : RestReplyBase(*new EvalReplyPrivate, std::move(requestFactory), std::move(policy), parent)
{ }

Core::Eval EvalReply::eval() const
{
    Q_D(const EvalReply);
    return d->eval;
}

bool EvalReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(EvalReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->eval = Core::Eval::fromJson(object);
    Q_EMIT finished(d->eval);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
