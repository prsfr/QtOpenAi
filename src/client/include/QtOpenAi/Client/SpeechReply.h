// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>

#include <QtCore/QByteArray>
#include <QtCore/QObject>

#include <functional>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class SpeechReplyPrivate;

// An asynchronous handle for one text-to-speech request (POST /audio/speech).
// The endpoint returns a binary audio blob, so the successful payload is exposed
// verbatim as raw bytes (with the response's Content-Type) rather than parsed as
// JSON. Emits finished() on success and failed() on error; both are followed by
// done(). Auto-deletes after done() unless disabled.
class QTOPENAI_CLIENT_EXPORT SpeechReply : public QObject
{
    Q_OBJECT
public:
    ~SpeechReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    // The raw audio bytes returned by the server (empty until finished).
    QByteArray audioData() const;
    // The response Content-Type, e.g. "audio/mpeg".
    QByteArray contentType() const;
    ClientError error() const;

    RateLimit rateLimit() const;
    int retryCount() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    void finished(const QByteArray &audioData);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();
    void retrying(int attempt, int delayMs);

private:
    friend class Client;
    SpeechReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(SpeechReply)
    QScopedPointer<SpeechReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
