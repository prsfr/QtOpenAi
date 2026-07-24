// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for one text-to-speech request (POST /audio/speech).
// The endpoint returns a binary audio blob, so the successful payload is exposed
// verbatim as raw bytes (with the response's Content-Type) rather than parsed as
// JSON. See RestReplyBase for the shared lifecycle (finished/failed/done,
// auto-delete).
class QTOPENAI_CLIENT_EXPORT SpeechReply : public RestReplyBase
{
    Q_OBJECT
public:
    // The raw audio bytes returned by the server (empty until finished).
    QByteArray audioData() const;
    // The response Content-Type, e.g. "audio/mpeg".
    QByteArray contentType() const;

Q_SIGNALS:
    void finished(const QByteArray &audioData);

private:
    friend class Client;
    SpeechReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    QByteArray m_audioData;
    QByteArray m_contentType;
};

} // namespace Client
} // namespace QtOpenAi
