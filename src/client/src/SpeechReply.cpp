// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/SpeechReply.h"

namespace QtOpenAi {
namespace Client {

SpeechReply::SpeechReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                         QObject *parent)
    : RestReplyBase(std::move(requestFactory), std::move(policy), parent)
{ }

QByteArray SpeechReply::audioData() const { return m_audioData; }

QByteArray SpeechReply::contentType() const { return m_contentType; }

bool SpeechReply::dispatchSuccess(const QByteArray &body, int)
{
    // Binary payload: surface the bytes verbatim, no JSON parsing.
    m_audioData = body;
    m_contentType = responseContentType();
    Q_EMIT finished(m_audioData);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
