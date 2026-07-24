// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ModerationReply.h"

namespace QtOpenAi {
namespace Client {

ModerationReply::ModerationReply(std::function<QNetworkReply *()> requestFactory,
                                 RetryPolicy policy, QObject *parent)
    : RestReplyBase(std::move(requestFactory), std::move(policy), parent)
{ }

Core::ModerationResponse ModerationReply::response() const { return m_response; }

bool ModerationReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    m_response = Core::ModerationResponse::fromJson(object);
    Q_EMIT finished(m_response);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
