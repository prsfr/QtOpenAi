// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ModelReply.h"

namespace QtOpenAi {
namespace Client {

ModelReply::ModelReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                       QObject *parent)
    : RestReplyBase(std::move(requestFactory), std::move(policy), parent)
{ }

Core::Model ModelReply::model() const { return m_model; }

bool ModelReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    m_model = Core::Model::fromJson(object);
    Q_EMIT finished(m_model);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
