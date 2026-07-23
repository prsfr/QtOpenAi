// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>
#include <QtOpenAi/Core/ModerationResponse.h>

#include <QtCore/QObject>

#include <functional>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class ModerationReplyPrivate;

// An asynchronous handle for one moderation request (POST /moderations). Emits
// finished() on success and failed() on error; both are followed by done().
// Auto-deletes after done() unless disabled.
class QTOPENAI_CLIENT_EXPORT ModerationReply : public QObject
{
    Q_OBJECT
public:
    ~ModerationReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    Core::ModerationResponse response() const;
    ClientError error() const;

    RateLimit rateLimit() const;
    int retryCount() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ModerationResponse &response);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();
    void retrying(int attempt, int delayMs);

private:
    friend class Client;
    ModerationReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                    QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(ModerationReply)
    QScopedPointer<ModerationReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
