// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ModelReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class ModelReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::Model model;
};

ModelReply::ModelReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                       QObject *parent)
    : RestReplyBase(*new ModelReplyPrivate, std::move(requestFactory), std::move(policy), parent)
{ }

Core::Model ModelReply::model() const
{
    Q_D(const ModelReply);
    return d->model;
}

bool ModelReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(ModelReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->model = Core::Model::fromJson(object);
    Q_EMIT finished(d->model);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
