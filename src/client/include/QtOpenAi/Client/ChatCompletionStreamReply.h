// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Core/ChatCompletionChunk.h>
#include <QtOpenAi/Core/ChatCompletionResponse.h>

#include <QtCore/QObject>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class ChatCompletionStreamReplyPrivate;

// An asynchronous handle for a streamed (`stream: true`) chat completion.
//
// As Server-Sent Events arrive it emits delta() for each parsed chunk and
// contentDelta() for the incremental text of the first choice. When the stream
// ends it emits finished() with the fully reassembled response (content
// concatenated, tool calls merged), or failed() on error; both precede done().
// The object deletes itself after done() unless setAutoDelete(false) was set.
class QTOPENAI_CLIENT_EXPORT ChatCompletionStreamReply : public QObject
{
    Q_OBJECT
public:
    ~ChatCompletionStreamReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    // The response reassembled from all chunks received so far.
    Core::ChatCompletionResponse response() const;
    ClientError error() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    void delta(const QtOpenAi::Core::ChatCompletionChunk &chunk);
    void contentDelta(const QString &text);
    void finished(const QtOpenAi::Core::ChatCompletionResponse &response);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();

private:
    friend class Client;
    explicit ChatCompletionStreamReply(QNetworkReply *reply, QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(ChatCompletionStreamReply)
    QScopedPointer<ChatCompletionStreamReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
