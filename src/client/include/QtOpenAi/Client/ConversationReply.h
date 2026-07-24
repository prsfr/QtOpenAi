// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/Conversation.h>

namespace QtOpenAi {
namespace Client {

class ConversationReplyPrivate;

// A conversation resource (/conversations); the reply also carries the
// acknowledgement returned by delete operations.
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ConversationReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::Conversation conversation() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Conversation &conversation);

private:
    friend class Client;
    ConversationReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                      QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(ConversationReply)
};

} // namespace Client
} // namespace QtOpenAi
