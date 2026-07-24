// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/CompletionResponse.h>

namespace QtOpenAi {
namespace Client {

// A legacy text completion (POST /completions).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT CompletionReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::CompletionResponse response() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::CompletionResponse &response);

private:
    friend class Client;
    CompletionReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                    QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Core::CompletionResponse m_response;
};

} // namespace Client
} // namespace QtOpenAi
