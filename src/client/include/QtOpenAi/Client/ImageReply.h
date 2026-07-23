// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>
#include <QtOpenAi/Core/ImageResponse.h>

#include <QtCore/QObject>

#include <functional>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class ImageReplyPrivate;

// An asynchronous handle for an images request (POST /images/generations,
// /edits or /variations). All three return the same shape, so this reply serves
// them all. Emits finished() on success and failed() on error; both are
// followed by done(). Auto-deletes after done() unless disabled.
class QTOPENAI_CLIENT_EXPORT ImageReply : public QObject
{
    Q_OBJECT
public:
    ~ImageReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    Core::ImageResponse response() const;
    ClientError error() const;

    RateLimit rateLimit() const;
    int retryCount() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ImageResponse &response);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();
    void retrying(int attempt, int delayMs);

private:
    friend class Client;
    ImageReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
               QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(ImageReply)
    QScopedPointer<ImageReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
