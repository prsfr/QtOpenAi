// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>

namespace QtOpenAi {
namespace Client {

class BinaryReplyPrivate;

// A reply for endpoints that return a raw binary blob rather than JSON. The
// bytes are surfaced verbatim together with the response Content-Type, exactly
// as the server sent them (no decoding). Used directly for generic content
// downloads (e.g. GET /files/{id}/content) and subclassed where a domain
// accessor name reads better (SpeechReply::audioData, VideoContentReply::videoData).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT BinaryReply : public RestReplyBase
{
    Q_OBJECT
public:
    // The raw bytes returned by the server (empty until finished).
    QByteArray data() const;
    // The response Content-Type, e.g. "audio/mpeg" or "video/mp4".
    QByteArray contentType() const;

Q_SIGNALS:
    void finished(const QByteArray &data);

protected:
    friend class Client;
    BinaryReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

private:
    Q_DECLARE_PRIVATE(BinaryReply)
};

} // namespace Client
} // namespace QtOpenAi
