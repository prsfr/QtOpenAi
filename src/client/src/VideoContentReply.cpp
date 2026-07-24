// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VideoContentReply.h"

namespace QtOpenAi {
namespace Client {

VideoContentReply::VideoContentReply(std::function<QNetworkReply *()> requestFactory,
                                     RetryPolicy policy, QObject *parent)
    : RestReplyBase(std::move(requestFactory), std::move(policy), parent)
{ }

QByteArray VideoContentReply::videoData() const { return m_videoData; }

QByteArray VideoContentReply::contentType() const { return m_contentType; }

bool VideoContentReply::dispatchSuccess(const QByteArray &body, int)
{
    // Binary payload: surface the bytes verbatim, no JSON parsing.
    m_videoData = body;
    m_contentType = responseContentType();
    Q_EMIT finished(m_videoData);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
