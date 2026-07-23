// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>
#include <QtOpenAi/Core/ChatCompletionList.h>

#include <QtCore/QObject>

#include <functional>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class ChatCompletionMessageListReplyPrivate;

// An asynchronous handle for a stored-completion messages list request
// (GET /chat/completions/{id}/messages). Emits finished() on success and
// failed() on error; both are followed by done(). Auto-deletes unless disabled.
class QTOPENAI_CLIENT_EXPORT ChatCompletionMessageListReply : public QObject
{
    Q_OBJECT
public:
    ~ChatCompletionMessageListReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    Core::ChatCompletionMessageList list() const;
    ClientError error() const;

    RateLimit rateLimit() const;
    int retryCount() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ChatCompletionMessageList &list);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();
    void retrying(int attempt, int delayMs);

private:
    friend class Client;
    ChatCompletionMessageListReply(std::function<QNetworkReply *()> requestFactory,
                                   RetryPolicy policy, QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(ChatCompletionMessageListReply)
    QScopedPointer<ChatCompletionMessageListReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
