// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>
#include <QtOpenAi/Core/Response.h>

#include <QtCore/QJsonObject>
#include <QtCore/QObject>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class ResponseStreamReplyPrivate;

// An asynchronous handle for a streamed (`stream: true`) Responses-API request.
//
// The Responses stream is a sequence of typed events. This reply surfaces the
// common ones as dedicated signals — outputTextDelta() and
// functionCallArgumentsDelta() — and every event (including ones without a
// dedicated signal) via event(type, data). When the terminal `response.completed`
// event arrives it emits finished() with the full Response; a `response.failed` /
// error event or an HTTP error emits failed(). Both precede done(). The object
// deletes itself after done() unless disabled.
class QTOPENAI_CLIENT_EXPORT ResponseStreamReply : public QObject
{
    Q_OBJECT
public:
    ~ResponseStreamReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    // The final Response from the `response.completed` event (default-constructed
    // until then).
    Core::Response response() const;
    ClientError error() const;

    RateLimit rateLimit() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    // Every streamed event, with its `type` and raw JSON payload.
    void event(const QString &type, const QJsonObject &data);
    // Incremental assistant text (`response.output_text.delta`).
    void outputTextDelta(const QString &text);
    // Incremental function-call arguments (`response.function_call_arguments.delta`).
    void functionCallArgumentsDelta(const QString &delta);
    void finished(const QtOpenAi::Core::Response &response);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();

private:
    friend class Client;
    explicit ResponseStreamReply(QNetworkReply *reply, QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(ResponseStreamReply)
    QScopedPointer<ResponseStreamReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
