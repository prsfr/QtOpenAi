// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>
#include <QtOpenAi/Core/Conversation.h>

#include <QtCore/QObject>

#include <functional>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class ConversationReplyPrivate;

// An asynchronous handle for a Conversations-API request whose result is a
// `conversation` object (create/get/update/delete). Emits finished() on success
// and failed() on error; both are followed by done(). Auto-deletes after done()
// unless setAutoDelete(false) was set.
class QTOPENAI_CLIENT_EXPORT ConversationReply : public QObject
{
    Q_OBJECT
public:
    ~ConversationReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    Core::Conversation conversation() const;
    ClientError error() const;

    RateLimit rateLimit() const;
    int retryCount() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Conversation &conversation);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();
    void retrying(int attempt, int delayMs);

private:
    friend class Client;
    ConversationReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                      QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(ConversationReply)
    QScopedPointer<ConversationReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
