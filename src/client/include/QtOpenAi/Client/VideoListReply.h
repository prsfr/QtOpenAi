// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/VideoJob.h>

namespace QtOpenAi {
namespace Client {

class VideoListReplyPrivate;

// A videos-list request (GET /videos).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT VideoListReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::VideoList list() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VideoList &list);

private:
    friend class Client;
    VideoListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                   QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(VideoListReply)
};

} // namespace Client
} // namespace QtOpenAi
