// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/ChatCompletionList.h>

namespace QtOpenAi {
namespace Client {

class ChatCompletionListReplyPrivate;

// A list of stored chat completions (GET /chat/completions).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ChatCompletionListReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::ChatCompletionList list() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ChatCompletionList &list);

private:
    friend class Client;
    ChatCompletionListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                            QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(ChatCompletionListReply)
};

} // namespace Client
} // namespace QtOpenAi
