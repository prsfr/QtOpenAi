// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>
#include <QtOpenAi/Core/TranscriptionResponse.h>

#include <QtCore/QObject>

#include <functional>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class TranscriptionReplyPrivate;

// An asynchronous handle for a speech-to-text request (POST
// /audio/transcriptions or /audio/translations). Both endpoints return the same
// shape, so this reply serves both. For the plain text/srt/vtt response formats
// the body is surfaced through response().text(); a JSON (verbose_json) body is
// parsed into segments/words. Emits finished() on success and failed() on error;
// both are followed by done(). Auto-deletes after done() unless disabled.
class QTOPENAI_CLIENT_EXPORT TranscriptionReply : public QObject
{
    Q_OBJECT
public:
    ~TranscriptionReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    Core::TranscriptionResponse response() const;
    ClientError error() const;

    RateLimit rateLimit() const;
    int retryCount() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    void finished(const QtOpenAi::Core::TranscriptionResponse &response);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();
    void retrying(int attempt, int delayMs);

private:
    friend class Client;
    TranscriptionReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                       QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(TranscriptionReply)
    QScopedPointer<TranscriptionReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
