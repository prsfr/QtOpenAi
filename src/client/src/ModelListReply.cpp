// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ModelListReply.h"

namespace QtOpenAi {
namespace Client {

ModelListReply::ModelListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                               QObject *parent)
    : RestReplyBase(std::move(requestFactory), std::move(policy), parent)
{ }

Core::ModelList ModelListReply::models() const { return m_models; }

bool ModelListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    m_models = Core::ModelList::fromJson(object);
    Q_EMIT finished(m_models);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
