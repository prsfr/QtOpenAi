// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/StreamReplyBase.h>
#include <QtOpenAi/Core/ChatCompletionChunk.h>
#include <QtOpenAi/Core/ChatCompletionResponse.h>

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
class QTOPENAI_CLIENT_EXPORT ChatCompletionStreamReply : public StreamReplyBase
{
    Q_OBJECT
public:
    // The response reassembled from all chunks received so far.
    Core::ChatCompletionResponse response() const;

Q_SIGNALS:
    void delta(const QtOpenAi::Core::ChatCompletionChunk &chunk);
    void contentDelta(const QString &text);
    void finished(const QtOpenAi::Core::ChatCompletionResponse &response);

private:
    friend class Client;
    explicit ChatCompletionStreamReply(QNetworkReply *reply, QObject *parent = nullptr);

    // See StreamReplyBase for what these are called with and when.
    void handleEvent(const QByteArray &name, const QByteArray &data) override;
    bool dispatchFinished(int httpStatus) override;

    Q_DECLARE_PRIVATE(ChatCompletionStreamReply)
};

} // namespace Client
} // namespace QtOpenAi
