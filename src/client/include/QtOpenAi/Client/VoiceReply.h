// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/Voice.h>

namespace QtOpenAi {
namespace Client {

class VoiceReplyPrivate;

// An asynchronous handle for POST /audio/voices, which builds a custom voice
// from an audio sample. See RestReplyBase for the shared lifecycle
// (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT VoiceReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::Voice voice() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Voice &voice);

private:
    friend class Client;
    VoiceReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
               QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(VoiceReply)
};

} // namespace Client
} // namespace QtOpenAi
