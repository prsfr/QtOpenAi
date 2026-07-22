// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Core/ChatCompletionResponse.h>

#include <QtCore/QObject>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class ChatCompletionReplyPrivate;

// An asynchronous handle for one chat-completion request. Emits finished() on
// success and failed() on error; both are followed by done(). The object
// deletes itself after done() is emitted unless setAutoDelete(false) was set.
class QTOPENAI_CLIENT_EXPORT ChatCompletionReply : public QObject
{
    Q_OBJECT
public:
    ~ChatCompletionReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    Core::ChatCompletionResponse response() const;
    ClientError error() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    // Abort the in-flight network request, if any.
    void abort();

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ChatCompletionResponse &response);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();

private:
    friend class Client;
    explicit ChatCompletionReply(QNetworkReply *reply, QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(ChatCompletionReply)
    QScopedPointer<ChatCompletionReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
