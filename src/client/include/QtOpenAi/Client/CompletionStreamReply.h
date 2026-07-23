// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>
#include <QtOpenAi/Core/CompletionResponse.h>

#include <QtCore/QObject>

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
class QTOPENAI_CLIENT_EXPORT CompletionStreamReply : public QObject
{
    Q_OBJECT
public:
    ~CompletionStreamReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    // The response reassembled from all chunks received so far.
    Core::CompletionResponse response() const;
    ClientError error() const;

    RateLimit rateLimit() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    void textDelta(const QString &text);
    void finished(const QtOpenAi::Core::CompletionResponse &response);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();

private:
    friend class Client;
    explicit CompletionStreamReply(QNetworkReply *reply, QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(CompletionStreamReply)
    QScopedPointer<CompletionStreamReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
