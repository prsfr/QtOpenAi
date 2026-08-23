// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class StreamReplyBasePrivate;

// Shared base for the Server-Sent-Event replies.
//
// It owns the network reply and the SSE framing, the state every stream carries
// (finished/success/error/rate limit/auto-delete), the common accessors and the
// type-independent signals, and it drives the whole lifecycle: frame each
// event, hand it to the subclass, and at the end either fail with what the
// response said or let the subclass emit its typed finished(...).
//
// A concrete stream is left with what is genuinely its own -- which events it
// recognises, what it accumulates, its typed getter and signals, and when it
// considers the stream complete:
//
//     class QTOPENAI_CLIENT_EXPORT ResponseStreamReply : public StreamReplyBase
//     {
//         Q_OBJECT
//     public:
//         Core::Response response() const;
//     Q_SIGNALS:
//         void finished(const QtOpenAi::Core::Response &response);
//     private:
//         friend class Client;
//         explicit ResponseStreamReply(QNetworkReply *reply, QObject *parent = nullptr);
//         void handleEvent(const QByteArray &name, const QByteArray &data) override;
//         bool dispatchFinished(int httpStatus) override;
//     };
//
// This is TypedReply's counterpart for the streaming half of the API. Before
// it, each of the four streams repeated the same ninety lines: the error-body
// handling, the rate-limit read, the auto-delete, and seven accessors that
// differed in nothing but the class name.
class QTOPENAI_CLIENT_EXPORT StreamReplyBase : public QObject
{
    Q_OBJECT
public:
    ~StreamReplyBase() override;

    bool isFinished() const;
    bool isSuccess() const;
    ClientError error() const;

    // Rate-limit information from the response headers.
    RateLimit rateLimit() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();

protected:
    // Subclasses pass a StreamReplyBasePrivate subclass holding whatever they
    // accumulate; the base's virtual destructor lets the one QScopedPointer own
    // it safely (the Qt d-pointer convention, as RestReplyBase does it).
    StreamReplyBase(StreamReplyBasePrivate &dd, QNetworkReply *reply, QObject *parent = nullptr);

    // One framed event, in arrival order. `name` is the SSE `event:` field --
    // empty for the streams that name their events inside the payload instead,
    // and the only signal for the Assistants run stream, which names them
    // nowhere else. `data` is the payload. The terminating [DONE] sentinel is
    // not passed on; no stream here does anything with it.
    virtual void handleEvent(const QByteArray &name, const QByteArray &data) = 0;

    // The stream ended and the transport reported no error. Emit the subclass's
    // typed finished(...) and return true; or record with setError() why what
    // arrived is not a complete answer and return false, and the base fails it.
    //
    // This is the one thing the streams genuinely disagree on at the end: a
    // chat or text completion is complete whenever the transport was, while a
    // response or a run has to have seen the event that says so.
    virtual bool dispatchFinished(int httpStatus) = 0;

    // Record why the stream is not a complete answer, for error()/failed().
    void setError(const ClientError &error);

    QScopedPointer<StreamReplyBasePrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(StreamReplyBase)
};

} // namespace Client
} // namespace QtOpenAi
