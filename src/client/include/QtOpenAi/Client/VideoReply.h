// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/VideoJob.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single-video request (POST /videos, GET
// /videos/{id}, POST /videos/{id}/remix, DELETE /videos/{id}). All return a
// VideoJob shape, so this reply serves them all. See RestReplyBase for the
// shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT VideoReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::VideoJob job() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VideoJob &job);

private:
    friend class Client;
    VideoReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
               QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Core::VideoJob m_job;
};

} // namespace Client
} // namespace QtOpenAi
