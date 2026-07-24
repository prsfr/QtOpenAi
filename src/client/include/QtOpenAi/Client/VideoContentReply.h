// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a rendered-video download (GET /videos/{id}/content).
// The endpoint returns a binary video blob, so the successful payload is exposed
// verbatim as raw bytes (with the response's Content-Type) rather than parsed as
// JSON, mirroring SpeechReply. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT VideoContentReply : public RestReplyBase
{
    Q_OBJECT
public:
    // The raw video bytes returned by the server (empty until finished).
    QByteArray videoData() const;
    // The response Content-Type, e.g. "video/mp4".
    QByteArray contentType() const;

Q_SIGNALS:
    void finished(const QByteArray &videoData);

private:
    friend class Client;
    VideoContentReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                      QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    QByteArray m_videoData;
    QByteArray m_contentType;
};

} // namespace Client
} // namespace QtOpenAi
