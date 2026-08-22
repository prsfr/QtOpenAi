// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/StreamReplyBase.h>
#include <QtOpenAi/Core/CompletionResponse.h>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class CompletionStreamReplyPrivate;

// An asynchronous handle for a streamed (`stream: true`) legacy text completion.
//
// As Server-Sent Events arrive it emits textDelta() for each incremental text
// fragment of the first choice. When the stream ends it emits finished() with
// the reassembled response (text concatenated), or failed() on error; both
// precede done(). The object deletes itself after done() unless disabled.
class QTOPENAI_CLIENT_EXPORT CompletionStreamReply : public StreamReplyBase
{
    Q_OBJECT
public:
    // The response reassembled from all chunks received so far.
    Core::CompletionResponse response() const;

Q_SIGNALS:
    void textDelta(const QString &text);
    void finished(const QtOpenAi::Core::CompletionResponse &response);

private:
    friend class Client;
    explicit CompletionStreamReply(QNetworkReply *reply, QObject *parent = nullptr);

    // See StreamReplyBase for what these are called with and when.
    void handleEvent(const QByteArray &name, const QByteArray &data) override;
    bool dispatchFinished(int httpStatus) override;

    Q_DECLARE_PRIVATE(CompletionStreamReply)
};

} // namespace Client
} // namespace QtOpenAi
