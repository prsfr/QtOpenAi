// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/ChatCompletionList.h>

namespace QtOpenAi {
namespace Client {

// The input messages of a stored chat completion
// (GET /chat/completions/{id}/messages).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ChatCompletionMessageListReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::ChatCompletionMessageList list() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ChatCompletionMessageList &list);

private:
    friend class Client;
    ChatCompletionMessageListReply(std::function<QNetworkReply *()> requestFactory,
                                   RetryPolicy policy, QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Core::ChatCompletionMessageList m_list;
};

} // namespace Client
} // namespace QtOpenAi
