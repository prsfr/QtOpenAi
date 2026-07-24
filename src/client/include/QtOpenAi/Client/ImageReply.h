// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/ImageResponse.h>

namespace QtOpenAi {
namespace Client {

// An images request (POST /images/generations, /edits or /variations).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ImageReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::ImageResponse response() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ImageResponse &response);

private:
    friend class Client;
    ImageReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
               QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Core::ImageResponse m_response;
};

} // namespace Client
} // namespace QtOpenAi
