// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/TranscriptionReply.h"

namespace QtOpenAi {
namespace Client {

TranscriptionReply::TranscriptionReply(std::function<QNetworkReply *()> requestFactory,
                                       RetryPolicy policy, QObject *parent)
    : RestReplyBase(std::move(requestFactory), std::move(policy), parent)
{ }

Core::TranscriptionResponse TranscriptionReply::response() const { return m_response; }

bool TranscriptionReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    m_response = Core::TranscriptionResponse::fromJson(object);
    Q_EMIT finished(m_response);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
