// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/EmbeddingReply.h"

namespace QtOpenAi {
namespace Client {

EmbeddingReply::EmbeddingReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                               QObject *parent)
    : RestReplyBase(std::move(requestFactory), std::move(policy), parent)
{ }

Core::EmbeddingResponse EmbeddingReply::response() const { return m_response; }

bool EmbeddingReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    m_response = Core::EmbeddingResponse::fromJson(object);
    Q_EMIT finished(m_response);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
