// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ModelListReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class ModelListReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::ModelList models;
};

ModelListReply::ModelListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                               QObject *parent)
    : RestReplyBase(*new ModelListReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::ModelList ModelListReply::models() const
{
    Q_D(const ModelListReply);
    return d->models;
}

bool ModelListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(ModelListReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->models = Core::ModelList::fromJson(object);
    Q_EMIT finished(d->models);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
