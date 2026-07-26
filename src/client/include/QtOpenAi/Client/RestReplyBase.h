// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>

#include <QtCore/QByteArray>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

#include <functional>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class RestReplyBasePrivate;

// Shared base for the typed one-shot replies.
//
// It owns the RestReply transport engine plus the reply state every typed reply
// carries (finished/success/error/auto-delete), exposes the common accessors and
// the type-independent signals (failed/done/retrying), and drives the whole
// success/failure lifecycle. A subclass adds only its typed value getter, its
// typed `finished(...)` signal, and a dispatchSuccess() override that decodes the
// 2xx body and emits that signal — so each concrete reply shrinks to a handful of
// lines instead of re-implementing the same plumbing.
class QTOPENAI_CLIENT_EXPORT RestReplyBase : public QObject
{
    Q_OBJECT
public:
    ~RestReplyBase() override;

    bool isFinished() const;
    bool isSuccess() const;
    ClientError error() const;

    RateLimit rateLimit() const;
    int retryCount() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();
    void retrying(int attempt, int delayMs);

protected:
    // Constructed by subclasses that need private state of their own, passing a
    // RestReplyBasePrivate subclass (the streaming replies do this).
    RestReplyBase(RestReplyBasePrivate &dd, std::function<QNetworkReply *()> requestFactory,
                  RetryPolicy policy, QObject *parent = nullptr);

    // Constructed by subclasses that add no private state, letting the base own
    // its data outright — the case for every TypedReply.
    RestReplyBase(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                  QObject *parent = nullptr);

    // Decode a 2xx response body and emit the subclass's typed finished(...)
    // signal. Return true on success; on a decode failure call setError() and
    // return false (the base then emits failed()). Invoked once per completed
    // request, after isFinished() is already true.
    virtual bool dispatchSuccess(const QByteArray &body, int httpStatus) = 0;

    // Helpers for dispatchSuccess() implementations:
    // Record a decode error to surface via error()/failed().
    void setError(const ClientError &error);
    // Parse `body` as a JSON object; on failure set a Parse error and return false.
    bool parseJsonObject(const QByteArray &body, int httpStatus, QJsonObject &out);
    // Content-Type of the successful response (for binary replies).
    QByteArray responseContentType() const;

    QScopedPointer<RestReplyBasePrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(RestReplyBase)
};

} // namespace Client
} // namespace QtOpenAi
