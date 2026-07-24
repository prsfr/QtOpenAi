// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/BinaryReply.h"

namespace QtOpenAi {
namespace Client {

BinaryReply::BinaryReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                         QObject *parent)
    : RestReplyBase(std::move(requestFactory), std::move(policy), parent)
{ }

QByteArray BinaryReply::data() const { return m_data; }

QByteArray BinaryReply::contentType() const { return m_contentType; }

bool BinaryReply::dispatchSuccess(const QByteArray &body, int)
{
    // Binary payload: surface the bytes verbatim, no JSON parsing.
    m_data = body;
    m_contentType = responseContentType();
    Q_EMIT finished(m_data);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
