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

class ChatCompletionListReplyPrivate;

// An asynchronous handle for a stored-chat-completions list request
// (GET /chat/completions). Emits finished() on success and failed() on error;
// both are followed by done(). Auto-deletes after done() unless disabled.
class QTOPENAI_CLIENT_EXPORT ChatCompletionListReply : public QObject
{
    Q_OBJECT
public:
    ~ChatCompletionListReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    Core::ChatCompletionList list() const;
    ClientError error() const;

    RateLimit rateLimit() const;
    int retryCount() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ChatCompletionList &list);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();
    void retrying(int attempt, int delayMs);

private:
    friend class Client;
    ChatCompletionListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                            QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(ChatCompletionListReply)
    QScopedPointer<ChatCompletionListReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
