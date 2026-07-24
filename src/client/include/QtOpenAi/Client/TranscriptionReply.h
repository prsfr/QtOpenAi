// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/TranscriptionResponse.h>

namespace QtOpenAi {
namespace Client {

class TranscriptionReplyPrivate;

// A speech-to-text request (POST /audio/transcriptions or /audio/translations).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT TranscriptionReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::TranscriptionResponse response() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::TranscriptionResponse &response);

private:
    friend class Client;
    TranscriptionReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                       QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(TranscriptionReply)
};

} // namespace Client
} // namespace QtOpenAi
