// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/ChatCompletionResponse.h>

namespace QtOpenAi {
namespace Client {

class ChatCompletionReplyPrivate;

// A chat completion (POST /chat/completions) or a stored completion
// retrieved/updated via /chat/completions/{id}.
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ChatCompletionReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::ChatCompletionResponse response() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ChatCompletionResponse &response);

private:
    friend class Client;
    ChatCompletionReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                        QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(ChatCompletionReply)
};

} // namespace Client
} // namespace QtOpenAi
