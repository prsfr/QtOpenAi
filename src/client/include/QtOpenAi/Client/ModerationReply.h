// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/ModerationResponse.h>

namespace QtOpenAi {
namespace Client {

class ModerationReplyPrivate;

// A moderation request (POST /moderations).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ModerationReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::ModerationResponse response() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ModerationResponse &response);

private:
    friend class Client;
    ModerationReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                    QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(ModerationReply)
};

} // namespace Client
} // namespace QtOpenAi
