// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/Response.h>

namespace QtOpenAi {
namespace Client {

// A Responses API request (POST/GET/DELETE/cancel /responses); on delete the
// response() carries the deletion acknowledgement.
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ResponseReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::Response response() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Response &response);

private:
    friend class Client;
    ResponseReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                  QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Core::Response m_response;
};

} // namespace Client
} // namespace QtOpenAi
